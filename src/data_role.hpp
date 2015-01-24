/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright (C) 2015 Mattia Basaglia
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef IMV_DATA_ROLE_HPP
#define IMV_DATA_ROLE_HPP

namespace imv {

/**
 * \brief Role to gather item data
 */
enum ItemDataRole
{
    Value,          ///< The value of the item
    Flags,          ///< Flags for allowed interaction
    Description,    ///< Item description

    UserRole = 0xf0,///< Use this or above for custom data
};

} // namespace imv
#endif // IMV_DATA_ROLE_HPP
