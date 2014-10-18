/*
 * This file is part of the KDE Baloo Project
 * Copyright (C) 2013  Vishesh Handa <me@vhanda.in>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef AGEPOSTINGSOURCE_H
#define AGEPOSTINGSOURCE_H

#include <xapian.h>
#include <QString>

namespace Baloo {

class AgePostingSource : public Xapian::ValuePostingSource
{
public:
    AgePostingSource(Xapian::valueno slot_);

    virtual Xapian::weight get_weight() const;
    virtual Xapian::PostingSource* clone() const;

    virtual std::string name() const {
        return "AgePostingSource";
    }

    virtual void init(const Xapian::Database& db_);

private:
    uint m_currentTime_t;
};

}

#endif // AGEPOSTINGSOURCE_H