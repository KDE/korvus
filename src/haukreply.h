// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>

#pragma once

#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QObject>
#include <QtCore/QUrl>

class HaukReplyData;
class HaukReply
{
    Q_GADGET
public:
    enum class Status { Invalid, Ok, Error };
    Q_ENUM(Status);

    HaukReply();
    HaukReply(const QString &response);
    HaukReply(const HaukReply &other);
    void swap(HaukReply &other) noexcept { d.swap(other.d); }
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(HaukReply);

    virtual ~HaukReply();

    Status status() const;

    QString error() const;

protected:
    QStringView data(qsizetype index) const;
    const QList<QStringView> &rawData() const;

private:
    QExplicitlySharedDataPointer<HaukReplyData> d;
};

class HaukCreateReply : public HaukReply
{
    Q_GADGET
public:
    using HaukReply::HaukReply;

    virtual QString session() const;

    QUrl url() const;

    QString groupPin() const;

    QString shareId() const;
};


class HaukPostReply : public HaukReply
{
    Q_GADGET
public:
    using HaukReply::HaukReply;

    QList<QUrl> urls() const;
};
