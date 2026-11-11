#pragma once
#include <QString>
class QWidget;
void showFileComparison(QWidget * parent, const QString & localPath,
                        const QString & remoteCopy, const QString & remoteLabel);
