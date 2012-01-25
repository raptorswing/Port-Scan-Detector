#include "PortEntryValidator.h"

#include <QStringList>

PortEntryValidator::PortEntryValidator(QObject *parent) :
    QValidator(parent)
{
}

QValidator::State PortEntryValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos)
    input.remove(QRegExp("[^,0123456789]"));

    QStringList ports = input.split(QChar(','),QString::SkipEmptyParts);
    foreach (QString sub, ports)
    {
        if (sub.length() > 5)
            return QValidator::Invalid;

        quint32 val = sub.toUInt();
        if (val > 65535)
            return QValidator::Invalid;
    }

    return QValidator::Acceptable;
}
