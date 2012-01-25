#ifndef PORTENTRYVALIDATOR_H
#define PORTENTRYVALIDATOR_H

#include <QValidator>

class PortEntryValidator : public QValidator
{
    Q_OBJECT
public:
    explicit PortEntryValidator(QObject *parent = 0);

    virtual QValidator::State validate(QString & input, int & pos) const;

signals:

public slots:

};

#endif // PORTENTRYVALIDATOR_H
