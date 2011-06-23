/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2011 by Eric MAEKER, MD (France) <eric.maeker@free.fr>        *
 *  All rights reserved.                                                   *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program (COPYING.FREEMEDFORMS file).                   *
 *  If not, see <http://www.gnu.org/licenses/>.                            *
 ***************************************************************************/
/***************************************************************************
 *   Main Developper : Eric MAEKER, <eric.maeker@free.fr>                  *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADRESS>                                                *
 *       NAME <MAIL@ADRESS>                                                *
 ***************************************************************************/
#ifndef CALENDARITEMMODEL_H
#define CALENDARITEMMODEL_H

#include <agendaplugin/agenda_exporter.h>
#include <calendar/abstract_calendar_model.h>

/**
 * \file calendaritemmodel.h
 * \author Eric MAEKER <eric.maeker@free.fr>
 * \version 0.6.0
 * \date 23 Jun 2011
*/

namespace Agenda {

class AGENDA_EXPORT CalendarItemModel : public Calendar::AbstractCalendarModel
{
    Q_OBJECT
public:
    CalendarItemModel(QObject *parent);
    ~CalendarItemModel();

    Calendar::CalendarItem getItemByUid(const QString &uid) const;
    QList<Calendar::CalendarItem> getItemsBetween(const QDate &from, const QDate &to) const;

    int count() const;

    const Calendar::CalendarItem &insertItem(const QDateTime &begin, const QDateTime &end);
    void setItemByUid(const QString &uid, const Calendar::CalendarItem &item);
    void removeItem(const QString &uid);

    void stopEvents();
    void resumeEvents();

public Q_SLOTS:
    void clearAll();

Q_SIGNALS:
    void itemInserted(const Calendar::CalendarItem &newItem);
    void itemModified(const Calendar::CalendarItem &oldItem, const Calendar::CalendarItem &newItem);
    void itemRemoved(const Calendar::CalendarItem &removedItem);
    void reset();

protected:
    void beginInsertItem();
    void endInsertItem(const Calendar::CalendarItem &newItem);
    void beginModifyItem();
    void endModifyItem(const Calendar::CalendarItem &oldItem, const Calendar::CalendarItem &newItem);
    void beginRemoveItem();
    void endRemoveItem(const Calendar::CalendarItem &removedItem);

private:
    int searchForIntersectedItem(const QList<Calendar::CalendarItem*> &list, const QDate &from, const QDate &to, int first, int last) const;
    int getInsertionIndex(bool begin, const QDateTime &dateTime, const QList<Calendar::CalendarItem*> &list, int first, int last) const;
    Calendar::CalendarItem *getItemPointerByUid(const QString &uid) const;
    QString createUid() const;

private:
    QList<Calendar::CalendarItem*> m_sortedByBeginList;
    QList<Calendar::CalendarItem*> m_sortedByEndList;
};

}  // End namespace Agenda

#endif // CALENDARITEMMODEL_H
