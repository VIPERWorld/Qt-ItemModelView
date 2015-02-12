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
#ifndef IMV_MODEL_HPP
#define IMV_MODEL_HPP

#include <QObject>
#include <QVariant>
#include "data_role.hpp"

namespace imv {


class Model;

/**
 * \brief Index of an item in a model
 */
class Index
{
public:
    /**
     * \brief Creates an invalid index
     */
    constexpr Index() = default;

    /**
     * \brief Row relative to parent()
     *
     * If the index is invalid, it will report -1
     */

    constexpr int row() const { return row_; }

    /**
     * \brief Column relative to parent()
     *
     * If the index is invalid, it will report -1
     */
    constexpr int column() const { return column_; }

    /**
     * \brief Model this index belongs to
     */
    constexpr const Model* model() const { return model_; }

    /**
     * \brief Internal ID representing the physical item
     * \note Shouldn't be used outside of model classes
     */
    constexpr quintptr internalId() const { return id_; }

    /**
     * \brief Internal pointer to the physical item
     * \note Shouldn't be used outside of model classes
     */
    void* internalPointer() const { return reinterpret_cast<void*>(id_); }

    /**
     * \brief Internal pointer to the physical item
     * \note Shouldn't be used outside of model classes
     */
    template<class T>
        T* internalPointer() const { return reinterpret_cast<T*>(id_); }

    /**
     * \brief Index to the parent item
     */
    inline Index parent() const;

    /**
     * \brief A child of the same parent
     */
    inline Index sibling(int row, int column) const;

    /**
     * \brief A child of the current index
     */
    inline Index child(int row, int column) const;

    /**
     * \brief Data of the item at this index
     */
    inline QVariant data(int role = Value) const;

    /**
     * \brief Number of child rows
     */
    inline int rowCount() const;

    /**
     * \brief Number of child columns
     */
    inline int columnCount() const;

    /**
     * \brief Whether it belongs to a model and points to an actual item
     * \note Some operations might render invalid indexes that are reported to be valid
     */
    inline bool valid() const;

    /**
     * \brief Compares two indices
     */
    constexpr bool operator==(const Index& rhs) const
    {
        return row_ == rhs.row_ && column_ == rhs.column_ &&
               id_ == rhs.id_ && model_ == rhs.model_;
    }

    constexpr bool operator!=(const Index& rhs) const
    {
        return !(*this == rhs);
    }

private:
    friend class Model;

    /**
     * \brief Creates a possibly valid index
     */
    constexpr Index(int row, int column, quintptr id, const Model* model)
        : row_(row), column_(column), id_(id), model_(model)
    {}

    int      row_ = -1;
    int      column_ = -1;
    quintptr id_ = 0;
    const Model* model_ = nullptr;
};


class Model : public QObject
{
    Q_OBJECT

private:
    enum Moving
    {
        Nothing,
        Rows,
        Columns
    };

public:
    virtual ~Model(){}

    int rowCount(const Index& parent = {}) const
    {
        return onRowCount(parent);
    }

    int columnCount(const Index& parent = {}) const
    {
        return onColumnCount(parent);
    }

    bool validRow(int row, const Index& parent) const
    {
        return row >= 0 && row <= rowCount(parent);
    }

    bool validColumn(int column, const Index& parent) const
    {
        return column >= 0 && column <= columnCount(parent);
    }

    bool valid(const Index& index) const
    {
        auto par = parent(index);
        return index.model() == this &&
               validRow(index.row(), par) &&
               validColumn(index.column(), par) &&
               onValid(index);
    }

    Index root() const
    {
        return onRoot();
    }

    Index index(int row, int column, const Index& parent = {}) const
    {
        if ( validRow(row, parent) && validColumn(column, parent) )
            return onIndex(row, column, parent);
        return {};
    }

    QVariant data(const Index& index, int role = Value) const
    {
        if ( !valid(index) )
            return QVariant();
        return onData(index, role);
    }

    bool setData(const Index& index, const QVariant& value, int role = Value)
    {
        if ( valid(index) && onSetData(index, value, role) )
        {
            emit dataChanged(index, value, role);
            return true;
        }
    }

    Index parent(const Index& index) const
    {
        if ( !valid(index) )
            return {};
        return onParent(index);
    }

    bool removeRow(int row, const Index& parent = {})
    {
        return removeRows(row, 1, parent);
    }

    bool removeRows(int row, int count, const Index& parent = {})
    {
        if ( count > 0 && validRow(row, parent) && validRow(row+count-1, parent)
                && onRemoveRows(row, count, parent) )
        {
            if ( !(moving_ & Rows) )
                emit rowsRemoved(row, count, parent);
            return true;
        }
        return false;
    }

    bool removeColumn(int column, const Index& parent = {})
    {
        return removeColumns(column, 1, parent);
    }

    bool removeColumns(int column, int count, const Index& parent = {})
    {
        if ( count > 0 && validColumn(column, parent) &&
            validColumn(column+count-1, parent) &&
            onRemoveColumns(column, count, parent) )
        {
            if ( !(moving_ & Columns) )
                emit columnsRemoved(column, count, parent);
            return true;
        }
        return false;
    }

    bool moveRow(const Index& from_parent, int from_row,
                 const Index& to_parent,   int to_row)
    {
        return moveRows(from_parent, from_row, 1, to_parent, to_row);
    }

    bool moveRows(const Index& from_parent, int from_row, int count,
                 const Index& to_parent, int to_row)
    {
        if ( count > 0 && validRow(from_row, from_parent) &&
            validRow(from_row+count-1, from_parent) )
        {
            beginMoveRows();
            bool ok = onMoveRows(from_parent, from_row, count, to_parent, to_row);
            endMoveRows(ok, from_parent, from_row, count, to_parent, to_row);
        }
        return false;
    }

    bool moveColumn(const Index& from_parent, int from_column,
                    const Index& to_parent,   int to_column)
    {
        return moveColumns(from_parent, from_column, 1, to_parent, to_column);
    }

    bool moveColumns(const Index& from_parent, int from_column, int count,
                     const Index& to_parent, int to_column)
    {
        if ( count > 0 && validColumn(from_column, from_parent) &&
            validColumn(from_column+count-1, from_parent) )
        {
            beginMoveColumns();
            bool ok = onMoveColumns(from_parent, from_column, count, to_parent, to_column);
            endMoveColumns(ok, from_parent, from_column, count, to_parent, to_column);
        }
        return false;
    }

protected:

    virtual bool onValid(const Index& index) const
    {
        return true;
    }

    virtual Index onRoot() const
    {
        return createIndex(-1, -1, 0);
    }

    virtual Index onIndex(int row, int column, const Index& parent) const
    {
        return createIndex(row, column, 0);
    }

    virtual bool onSetData(const Index& index, const QVariant& value, int role)
    {
        return false;
    }

    virtual QVariant onData(const Index& index, int role) const = 0;

    virtual int onRowCount(const Index& parent) const = 0;
    virtual int onColumnCount(const Index& parent) const = 0;

    virtual Index onParent(const Index& index) const
    {
        return {};
    }

    virtual bool onRemoveRows(int row, int count, const Index& parent)
    {
        return false;
    }

    virtual bool onRemoveColumns(int column, int count, const Index& parent)
    {
        return false;
    }

    virtual bool onMoveRows(const Index& from_parent, int from_row, int count,
                            const Index& to_parent, int to_row)
    {
        return false;
    }

    virtual bool onMoveColumns(const Index& from_parent, int from_column,
                               int count, const Index& to_parent, int to_column)
    {
        return false;
    }

    Index createIndex(int row, int column, quintptr id) const
    {
        return Index(row, column, id, this);
    }

    template<class T>
        Index createIndex(int row, int column, T* data) const
        {
            return Index(row, column, reinterpret_cast<quintptr>(data), this);
        }

    void beginMoveRows()
    {
        moving_ |= Rows;
    }

    void endMoveRows(bool ok, const Index& from_parent, int from_row, int count,
                     const Index& to_parent, int to_row)
    {
        moving_ &= ~Rows;
        if ( ok )
            emit rowsMoved(from_parent, from_row, count, to_parent, to_row);
    }

    void beginMoveColumns()
    {
        moving_ |= Columns;
    }

    void endMoveColumns(bool ok, const Index& from_parent, int from_column,
                        int count, const Index& to_parent, int to_column)
    {
        moving_ &= ~Columns;
        if ( ok )
            emit columnsMoved(from_parent, from_column, count, to_parent, to_column);
    }

signals:
    void dataChanged(const Index& index, const QVariant& value, int role);
    void rowsRemoved(int row, int count, const Index& parent);
    void columnsRemoved(int column, int count, const Index& parent);
    void rowsMoved(const Index& from_parent, int from_row, int count, const Index& to_parent, int to_row);
    void columnsMoved(const Index& from_parent, int from_column, int count, const Index& to_parent, int to_column);

private:
    bool moving_ = Nothing;
};


inline Index Index::child(int row, int column) const
{
    return model_ ? model_->index(row, column, *this) : Index();
}

inline int Index::columnCount() const
{
    return model_ ? model_->columnCount(*this) : 0;
}

inline QVariant Index::data(int role) const
{
    return model_ ? model_->data(*this, role) : QVariant();
}

inline Index Index::parent() const
{
    return model_ ? model_->parent(*this) : Index();
}

inline int Index::rowCount() const
{
    return model_ ? model_->rowCount(*this) : 0;
}

inline Index Index::sibling(int row, int column) const
{
    return model_ ? model_->index(row, column, model_->parent(*this)) : Index();
}

bool Index::valid() const
{
    return model_ ? model_->valid(*this) : false;
}

} // namespace imv
#endif // IMV_MODEL_HPP
