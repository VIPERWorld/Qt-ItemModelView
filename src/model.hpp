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

/**
 * \brief Base class for index models
 */
class Model : public QObject
{
    Q_OBJECT

private:
    /**
     * \brief Used to keep track of move rows/columns
     */
    enum Moving
    {
        Nothing,
        Rows,
        Columns
    };

public:
    virtual ~Model(){}

    /**
     * \brief Number of rows at the given parent
     */
    int rowCount(const Index& parent = {}) const
    {
        return onRowCount(parent);
    }

    /**
     * \brief Number of columns at the given parent
     */
    int columnCount(const Index& parent = {}) const
    {
        return onColumnCount(parent);
    }

    /**
     * \brief Whether a row is a valid child of parent
     */
    bool validRow(int row, const Index& parent) const
    {
        return row >= 0 && row <= rowCount(parent);
    }

    /**
     * \brief Whether a column is a valid child of parent
     */
    bool validColumn(int column, const Index& parent) const
    {
        return column >= 0 && column <= columnCount(parent);
    }

    /**
     * \brief Whether an index is valid
     */
    bool valid(const Index& index) const
    {
        auto par = parent(index);
        return index.model() == this &&
               validRow(index.row(), par) &&
               validColumn(index.column(), par) &&
               onValid(index);
    }

    /**
     * \brief Returns an invalid index that belongs to the model
     */
    Index root() const
    {
        return onRoot();
    }

    /**
     * \brief Retuens a valid model index if the parameters are correct
     */
    Index index(int row, int column, const Index& parent = {}) const
    {
        if ( validRow(row, parent) && validColumn(column, parent) )
            return onIndex(row, column, parent);
        return {};
    }

    /**
     * \brief Returns the data associated with the item index for the given role
     * \returns An empty variant if the index or role are invalid or don't
     *          have any data to report
     */
    QVariant data(const Index& index, int role = Value) const
    {
        if ( !valid(index) )
            return QVariant();
        return onData(index, role);
    }

    /**
     * \brief Sets data for the item
     * \returns \b true on success
     *
     * Emits dataChanged() on success.
     */
    bool setData(const Index& index, const QVariant& value, int role = Value)
    {
        if ( valid(index) && onSetData(index, value, role) )
        {
            emit dataChanged(index, value, role);
            return true;
        }
        return false;
    }

    /**
     * \brief Returns the parent for that index
     */
    Index parent(const Index& index) const
    {
        if ( !valid(index) )
            return {};
        return onParent(index);
    }

    /**
     * \brief Removes a single row
     * \see removeRows
     */
    bool removeRow(int row, const Index& parent = {})
    {
        return removeRows(row, 1, parent);
    }

    /**
     * \brief Removes some rows
     * \returns \b true on success
     *
     * Emits rowsRemoved() on success when this
     * isn't being done while moving rows.
     */
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

    /**
     * \brief Removes a single column
     * \see removeColumns
     */
    bool removeColumn(int column, const Index& parent = {})
    {
        return removeColumns(column, 1, parent);
    }

    /**
     * \brief Removes some columns
     * \returns \b true on success
     *
     * Emits columnsRemoved() on success when this
     * isn't being done while moving column.
     */
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

    /**
     * \brief Moves a single row
     * \see moveRows
     */
    bool moveRow(const Index& from_parent, int from_row,
                 const Index& to_parent,   int to_row)
    {
        return moveRows(from_parent, from_row, 1, to_parent, to_row);
    }

    /**
     * \brief Moves some rows
     * \returns \b true on success
     *
     * Emits rowsMoved() on success.
     */
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

    /**
     * \brief Moves a single column
     * \see moveColumns
     */
    bool moveColumn(const Index& from_parent, int from_column,
                    const Index& to_parent,   int to_column)
    {
        return moveColumns(from_parent, from_column, 1, to_parent, to_column);
    }


    /**
     * \brief Moves some columns
     * \returns \b true on success
     *
     * Emits columnsMoved() on success.
     */
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

    /**
     * \brief Extra checks for the validity of \p index
     * \param index An index that is contained by the model
     *              and has valid row and column numbers
     */
    virtual bool onValid(const Index& index) const
    {
        return true;
    }

    /**
     * \brief Used to create an invalid index that represents the root
     */
    virtual Index onRoot() const
    {
        return createIndex(-1, -1, 0);
    }

    /**
     * \brief Used when an index is being requested
     * \param row       A valid row in \p parent
     * \param column    A valid column in \p parent
     * \param parent    Should be the result of parent(index) on the returned index
     * \return An index created by createIndex() or an invalid index
     */
    virtual Index onIndex(int row, int column, const Index& parent) const
    {
        return createIndex(row, column, 0);
    }

    /**
     * \brief Set the data to its destination
     * \param index A valid index in the model
     */
    virtual bool onSetData(const Index& index, const QVariant& value, int role)
    {
        return false;
    }

    /**
     * \brief Return the data associated with the index
     * \param index A valid index
     */
    virtual QVariant onData(const Index& index, int role) const = 0;

    /**
     * \brief Number of rows in \p parent
     *
     * If \p parent is invalid, this should return the number of
     * top-level rows in the model
     */
    virtual int onRowCount(const Index& parent) const = 0;

    /**
     * \brief Number of columns in \p parent
     *
     * If \p parent is invalid, this should return the number of
     * top-level columns in the model
     */
    virtual int onColumnCount(const Index& parent) const = 0;

    /**
     * \brief Returns the parent of \p index
     * \param index A valid index in the model
     */
    virtual Index onParent(const Index& index) const
    {
        return {};
    }

    /**
     * \brief Removes some rows from the model
     * \param row    A valid row in \p parent
     * \param count  Number of rows, already checked that they are all valid in \p parent
     * \param parent Parent index to remove from
     */
    virtual bool onRemoveRows(int row, int count, const Index& parent)
    {
        return false;
    }

    /**
     * \brief Removes some columns from the model
     * \param column A valid column in \p parent
     * \param count  Number of columns, already checked that they are all valid in \p parent
     * \param parent Parent index to remove from
     */
    virtual bool onRemoveColumns(int column, int count, const Index& parent)
    {
        return false;
    }

    /**
     * \brief Move some rows
     * \param from_parent   Parent containing the rows to be moved (may be invalid)
     * \param row           A valid row in \p from_parent
     * \param count         Number of rows, already checked that they are
     *                      all valid in \p from_parent
     * \param to_parent     Destination parent (may be invalid)
     * \param to_row        Destination row in \p to_parent (may be invalid)
     * \note This is called by moveRows() between beginMoveRows() and
     *       endMoveRows() which means rowsRemoved() and rowsAdded() won't
     *       be emitted automatically during the execution of this function.
     *       If some operation fails, this function must ensure that the
     *       model is back to the stating state or that the signals representing
     *       the operations that have been successful have been emitted.
     * \return \b true on success
     */
    virtual bool onMoveRows(const Index& from_parent, int from_row, int count,
                            const Index& to_parent, int to_row)
    {
        return false;
    }

    /**
     * \brief Move some columns
     * \param from_parent   Parent containing the columns to be moved (may be invalid)
     * \param column        A valid column in \p from_parent
     * \param count         Number of columns, already checked that they are
     *                      all valid in \p from_parent
     * \param to_parent     Destination parent (may be invalid)
     * \param to_column     Destination column in \p to_parent (may be invalid)
     * \note This is called by moveColumn() between beginMoveRows() and
     *       endMoveColumns() which means ColumnsRemoved() and rowsAdded() won't
     *       be emitted automatically during the execution of this function.
     *       If some operation fails, this function must ensure that the
     *       model is back to the stating state or that the signals representing
     *       the operations that have been successful have been emitted.
     * \return \b true on success
     */
    virtual bool onMoveColumns(const Index& from_parent, int from_column,
                               int count, const Index& to_parent, int to_column)
    {
        return false;
    }

    /**
     * \brief Creates an index belonging to this model
     */
    Index createIndex(int row, int column, quintptr id) const
    {
        return Index(row, column, id, this);
    }

    /**
     * \brief Creates an index belonging to this model
     */
    template<class T>
        Index createIndex(int row, int column, T* data) const
        {
            return Index(row, column, reinterpret_cast<quintptr>(data), this);
        }

    /**
     * \brief Begins a move operation
     *
     * This will inhibit the automatic emission of rowsRemoved() and
     * rowsAdded() so that a move can be seen as a single operation
     */
    void beginMoveRows()
    {
        moving_ |= Rows;
    }

    /**
     * \brief Ends a move operation
     *
     * This will restore the automatic emission of rowsRemoved() and
     * rowsAdded(). If \p ok is \b true, it will emit rowsMoved()
     */
    void endMoveRows(bool ok, const Index& from_parent, int from_row, int count,
                     const Index& to_parent, int to_row)
    {
        moving_ &= ~Rows;
        if ( ok )
            emit rowsMoved(from_parent, from_row, count, to_parent, to_row);
    }

    /**
     * \brief Begins a move operation
     *
     * This will inhibit the automatic emission of columnsRemoved() and
     * columnsAdded() so that a move can be seen as a single operation
     */
    void beginMoveColumns()
    {
        moving_ |= Columns;
    }

    /**
     * \brief Ends a move operation
     *
     * This will restore the automatic emission of columnsRemoved() and
     * columnsAdded(). If \p ok is \b true, it will emit columnsMoved()
     */
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
    void rowsAdded(int row, int count, const Index& parent);
    void columnsRemoved(int column, int count, const Index& parent);
    void columnsAdded(int row, int count, const Index& parent);
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
