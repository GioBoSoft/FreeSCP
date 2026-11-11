#include "filecompare.h"
#include <QDialog>
#include <QFile>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QScrollBar>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFontDatabase>
#include <QRegularExpression>
#include <memory>
#include <QStringDecoder>
#include <QProcess>
#include <QTemporaryDir>

namespace {
bool readText(const QString & path, QByteArray & bytes, QString & text, QString & error)
{
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) { error = f.errorString(); return false; }
  if (f.size() > 8 * 1024 * 1024) { error = "Comparison is limited to 8 MiB per file."; return false; }
  bytes = f.readAll();
  if (f.error() != QFileDevice::NoError) { error = f.errorString(); return false; }
  auto encoding = QStringConverter::encodingForData(bytes);
  QStringDecoder decoder(encoding.value_or(QStringConverter::Utf8));
  text = decoder(bytes);
  if (decoder.hasError() || text.contains(QChar(0))) text = QString();
  return true;
}
}

void showFileComparison(QWidget * parent, const QString & localPath,
                        const QString & remoteCopy, const QString & remoteLabel)
{
  QByteArray a, b; QString x, y, error;
  if (!readText(localPath, a, x, error) || !readText(remoteCopy, b, y, error))
  { QMessageBox::warning(parent, "Compare files", error); return; }
  if (a == b) { QMessageBox::information(parent, "Compare files", "The files are identical."); return; }
  if ((x.isNull() && !a.isEmpty()) || (y.isNull() && !b.isEmpty()))
  { QMessageBox::information(parent, "Compare files", "The files differ. At least one is binary or uses an unsupported text encoding."); return; }
  // Compare decoded text without altering line endings or whitespace.
  QTemporaryDir temp;
  if (!temp.isValid()) { QMessageBox::warning(parent, "Compare files", "Cannot create temporary directory."); return; }
  QStringList paths;
  for (const auto & pair : {qMakePair(QString("left"), x), qMakePair(QString("right"), y)})
  {
    QString path = temp.filePath(pair.first); QFile f(path); QByteArray data = pair.second.toUtf8();
    if (!f.open(QIODevice::WriteOnly) || f.write(data) != data.size())
    { QMessageBox::warning(parent, "Compare files", "Cannot write temporary file."); return; }
    paths << path;
  }
  // POSIX diff's normal format supplies line ranges; no shell or user command is executed.
  QProcess diff; diff.start("diff", paths);
  if (!diff.waitForFinished(10000) || diff.exitStatus() != QProcess::NormalExit || diff.exitCode() > 1)
  { diff.kill(); diff.waitForFinished(); QMessageBox::warning(parent, "Compare files", "Could not run diff (required on PATH), or comparison timed out."); return; }
  QStringList left = x.split('\n'), right = y.split('\n');
  if (x.endsWith('\n')) left.removeLast();
  if (y.endsWith('\n')) right.removeLast();
  if (x.isEmpty()) left.clear();
  if (y.isEmpty()) right.clear();
  QStringList displayLeft, displayRight; QList<int> changed;
  int li = 0, ri = 0;
  auto append = [&](bool l, bool r, bool change) {
    if (change) changed << displayLeft.size();
    displayLeft << (l ? QString::number(li + 1) + "  " + left[li++] : QString());
    displayRight << (r ? QString::number(ri + 1) + "  " + right[ri++] : QString());
  };
  QRegularExpression header("^(\\d+)(?:,(\\d+))?([acd])(\\d+)(?:,(\\d+))?$");
  for (const QString & line : QString::fromUtf8(diff.readAllStandardOutput()).split('\n'))
  {
    auto m = header.match(line); if (!m.hasMatch()) continue;
    int ls = m.captured(1).toInt(), le = m.captured(2).isEmpty() ? ls : m.captured(2).toInt();
    int rs = m.captured(4).toInt(), re = m.captured(5).isEmpty() ? rs : m.captured(5).toInt();
    QString op = m.captured(3);
    int beforeL = op == "a" ? ls : ls - 1, beforeR = op == "d" ? rs : rs - 1;
    while (li < beforeL || ri < beforeR) append(li < beforeL, ri < beforeR, false);
    int endL = op == "a" ? li : le, endR = op == "d" ? ri : re;
    while (li < endL || ri < endR) append(li < endL, ri < endR, true);
  }
  while (li < left.size() || ri < right.size()) append(li < left.size(), ri < right.size(), false);
  auto * window = new QDialog(parent); window->setAttribute(Qt::WA_DeleteOnClose);
  window->setWindowTitle("Compare files — Local / Remote"); window->resize(1100, 700);
  auto * layout = new QVBoxLayout(window);
  auto * label = new QLabel(localPath + "\n" + remoteLabel); label->setTextFormat(Qt::PlainText); layout->addWidget(label);
  auto * splitter = new QSplitter; layout->addWidget(splitter);
  QList<QPlainTextEdit *> views;
  for (const QStringList & lines : {displayLeft, displayRight})
  {
    auto * view = new QPlainTextEdit; view->setReadOnly(true); view->setLineWrapMode(QPlainTextEdit::NoWrap);
    view->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont)); view->setPlainText(lines.join('\n'));
    QList<QTextEdit::ExtraSelection> highlights;
    for (int row : changed) {
      QTextEdit::ExtraSelection s; s.cursor = QTextCursor(view->document()->findBlockByNumber(row));
      s.format.setBackground(QColor(110, 75, 0)); s.format.setForeground(Qt::white);
      s.format.setProperty(QTextFormat::FullWidthSelection, true); highlights << s;
    }
    view->setExtraSelections(highlights); splitter->addWidget(view); views << view;
  }
  for (int i = 0; i < 2; ++i)
    QObject::connect(views[i]->verticalScrollBar(), &QScrollBar::valueChanged, views[1-i]->verticalScrollBar(), &QScrollBar::setValue);
  auto * buttons = new QDialogButtonBox; layout->addWidget(buttons);
  auto current = std::make_shared<int>(-1);
  for (int direction : {-1, 1}) {
    auto * button = buttons->addButton(direction < 0 ? "Previous difference" : "Next difference", QDialogButtonBox::ActionRole);
    button->setEnabled(!changed.isEmpty());
    QObject::connect(button, &QPushButton::clicked, window, [views, changed, current, direction] {
      *current = *current < 0 ? (direction > 0 ? 0 : changed.size() - 1) : (*current + direction + changed.size()) % changed.size();
      for (auto * view : views) { view->setTextCursor(QTextCursor(view->document()->findBlockByNumber(changed[*current]))); view->centerCursor(); }
    });
  }
  layout->addWidget(new QLabel(changed.isEmpty() ? "Text is identical; encoding or byte representation differs." : "Highlighted rows differ. Comparison is read-only."));
  window->show();
}
