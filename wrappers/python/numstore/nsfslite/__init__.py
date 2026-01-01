"""
nsfslite - NumStore File System Lite Python bindings

A Python library for interacting with NumStore file system with byte-oriented storage.
"""

from . import _nsfslite
from typing import Optional, Union
import numpy as np


class Transaction:
    """Represents a transaction in the nsfslite database."""

    def __init__(self, handle, conn):
        self._handle = handle
        self._conn = conn
        self._committed = False

    def commit(self):
        """Commit the transaction."""
        if not self._committed:
            _nsfslite.commit_transaction(self._conn._handle, self._handle)
            self._committed = True

    def __enter__(self):
        """Context manager entry."""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit - auto-commit on success."""
        if exc_type is None and not self._committed:
            self.commit()
        return False


class Variable:
    """Represents a variable in the file system database."""

    def __init__(self, var_id, conn):
        self._id = var_id
        self._conn = conn

    def __len__(self):
        """Get the length of the variable."""
        return _nsfslite.variable_len(self._conn._handle, self._id)

    def insert(self, offset: int, data: Union[bytes, str, np.ndarray], txn: Optional[Transaction] = None):
        """Insert data at the specified offset.

        Args:
            offset: The offset at which to insert data
            data: Data to insert (can be bytes, string, numpy array, etc.)
            txn: Optional transaction to use for this operation
        """
        # Convert various types to bytes
        if isinstance(data, str):
            data_bytes = data.encode('utf-8')
        elif isinstance(data, np.ndarray):
            data_bytes = data.tobytes()
        elif isinstance(data, (bytes, bytearray)):
            data_bytes = bytes(data)
        else:
            # Try to convert to bytes
            data_bytes = bytes(data)

        if txn is not None:
            _nsfslite.variable_insert_tx(self._conn._handle, self._id, txn._handle, offset, data_bytes)
        else:
            _nsfslite.variable_insert(self._conn._handle, self._id, offset, data_bytes)

    def __getitem__(self, key):
        """Read data using slice notation: tv[start:stop:step]."""
        if isinstance(key, slice):
            start = key.start if key.start is not None else 0
            stop = key.stop if key.stop is not None else len(self)
            step = key.step if key.step is not None else 1

            data_bytes = _nsfslite.variable_read(self._conn._handle, self._id, start, stop, step)
            return data_bytes
        else:
            raise TypeError("Variable indices must be slices")

    def __setitem__(self, key, value):
        """Write data using slice notation: tv[start:stop:step] = data."""
        if isinstance(key, slice):
            start = key.start if key.start is not None else 0
            stop = key.stop if key.stop is not None else len(self)
            step = key.step if key.step is not None else 1

            # Convert various types to bytes
            if isinstance(value, str):
                value_bytes = value.encode('utf-8')
            elif isinstance(value, np.ndarray):
                value_bytes = value.tobytes()
            elif isinstance(value, (bytes, bytearray)):
                value_bytes = bytes(value)
            else:
                value_bytes = bytes(value)

            _nsfslite.variable_write(self._conn._handle, self._id, start, stop, step, value_bytes)
        else:
            raise TypeError("Variable indices must be slices")

    def write(self, start: int, stop: int, step: int, data: Union[bytes, str, np.ndarray], txn: Optional[Transaction] = None):
        """Write data with stride pattern.

        Args:
            start: Starting byte offset
            stop: Ending byte offset (exclusive)
            step: Step size in bytes
            data: Data to write
            txn: Optional transaction to use for this operation
        """
        # Convert various types to bytes
        if isinstance(data, str):
            data_bytes = data.encode('utf-8')
        elif isinstance(data, np.ndarray):
            data_bytes = data.tobytes()
        elif isinstance(data, (bytes, bytearray)):
            data_bytes = bytes(data)
        else:
            data_bytes = bytes(data)

        if txn is not None:
            _nsfslite.variable_write_tx(self._conn._handle, self._id, txn._handle, start, stop, step, data_bytes)
        else:
            _nsfslite.variable_write(self._conn._handle, self._id, start, stop, step, data_bytes)

    def __delitem__(self, key):
        """Remove data using slice notation: del tv[start:stop:step]."""
        if isinstance(key, slice):
            start = key.start if key.start is not None else 0
            stop = key.stop if key.stop is not None else len(self)
            step = key.step if key.step is not None else 1

            _nsfslite.variable_remove(self._conn._handle, self._id, start, stop, step, False)
        else:
            raise TypeError("Variable indices must be slices")

    def remove(self, start: int = None, stop: int = None, step: int = None, txn: Optional[Transaction] = None):
        """Remove data from variable.

        If called without arguments, returns a RemoveProxy for slice-based removal.
        If called with arguments, removes the specified range.

        Args:
            start: Starting byte offset (optional)
            stop: Ending byte offset (optional)
            step: Step size in bytes (optional)
            txn: Optional transaction to use for this operation
        """
        if start is None and stop is None and step is None:
            return RemoveProxy(self)

        start = start if start is not None else 0
        stop = stop if stop is not None else len(self)
        step = step if step is not None else 1

        if txn is not None:
            _nsfslite.variable_remove_tx(self._conn._handle, self._id, txn._handle, start, stop, step, False)
        else:
            _nsfslite.variable_remove(self._conn._handle, self._id, start, stop, step, False)


class RemoveProxy:
    """Proxy object for remove operations that return deleted data."""

    def __init__(self, variable):
        self._variable = variable

    def __getitem__(self, key):
        """Remove and return data using slice notation."""
        if isinstance(key, slice):
            start = key.start if key.start is not None else 0
            stop = key.stop if key.stop is not None else len(self._variable)
            step = key.step if key.step is not None else 1

            data_bytes = _nsfslite.variable_remove(self._variable._conn._handle, self._variable._id, start, stop, step, True)
            return data_bytes
        else:
            raise TypeError("Variable indices must be slices")


class Connection:
    """Connection to a nsfslite database."""

    def __init__(self, db_path: str, wal_path: str):
        self._handle = _nsfslite.open(db_path, wal_path)
        self._db_path = db_path
        self._wal_path = wal_path

    def create(self, name: str) -> Variable:
        """Create a new variable with the specified name.

        Args:
            name: The name of the variable

        Returns:
            Variable object
        """
        var_id = _nsfslite.create_variable(self._handle, name)
        return Variable(var_id, self)

    def get(self, name: str) -> Variable:
        """Get an existing variable by name.

        Args:
            name: The name of the variable

        Returns:
            Variable object
        """
        var_id = _nsfslite.get_variable(self._handle, name)
        return Variable(var_id, self)

    def delete(self, name: str):
        """Delete a variable by name.

        Args:
            name: The name of the variable to delete
        """
        _nsfslite.delete_variable(self._handle, name)

    def begin_transaction(self) -> Transaction:
        """Begin a new transaction.

        Returns:
            Transaction object that can be used as a context manager
        """
        txn_handle = _nsfslite.begin_transaction(self._handle)
        return Transaction(txn_handle, self)

    def close(self):
        """Close the database connection."""
        if self._handle is not None:
            _nsfslite.close(self._handle)
            self._handle = None

    def __enter__(self):
        """Context manager entry."""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.close()
        return False

    def __del__(self):
        """Cleanup on deletion."""
        self.close()


def open(db_path: str, wal_path: str) -> Connection:
    """Open a connection to a nsfslite database.

    Args:
        db_path: Path to the database file
        wal_path: Path to the WAL (Write-Ahead Log) file

    Returns:
        Connection object
    """
    return Connection(db_path, wal_path)


__all__ = ['open', 'Connection', 'Variable', 'Transaction']
