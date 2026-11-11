#include "../filecompare.h"
#include <QApplication>
#include <QPlainTextEdit>
#include <QDialog>
#include <QFile>
#include <QTemporaryDir>
#include <cassert>
int main(int argc, char ** argv) {
  QApplication app(argc, argv); QTemporaryDir dir;
  auto check = [&](QByteArray a, QByteArray b, int highlighted) {
    for (auto pair : {qMakePair(QString("a"), a), qMakePair(QString("b"), b)}) {
      QFile f(dir.filePath(pair.first)); assert(f.open(QIODevice::WriteOnly)); f.write(pair.second);
    }
    showFileComparison(nullptr, dir.filePath("a"), dir.filePath("b"), "remote");
    QDialog * dialog = nullptr;
    for (auto * w : QApplication::topLevelWidgets()) if (auto * d = qobject_cast<QDialog *>(w)) dialog = d;
    assert(dialog); auto views = dialog->findChildren<QPlainTextEdit *>(); assert(views.size() == 2);
    assert(views[0]->blockCount() == views[1]->blockCount());
    assert(views[0]->extraSelections().size() == highlighted);
    assert(views[0]->isReadOnly() && views[1]->isReadOnly()); delete dialog;
  };
  check("one\nthree\n", "one\ntwo\nthree\n", 1);
  check("one\ntwo\nthree\n", "one\nthree\n", 1);
  check("a\nb\n", "c\nd\n", 2);
  check("", "new\n", 1);
  check("old\n", "", 1);
  check("same", "same\n", 1);
}
