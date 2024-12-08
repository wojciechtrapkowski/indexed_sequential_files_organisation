# ISAM Database Implementation

This project is an implementation of the Indexed Sequential Access Method (ISAM) database structure, developed as part of the Database Structures course. 

## Overview

ISAM is a method for managing and accessing data in a file system that combines the benefits of sequential and indexed access methods. This implementation features:

- Index area for fast record lookup
- Main area for primary data storage
- Overflow area for handling insertions
- Configurable page sizes and buffer management
- Support for basic database operations (CRUD)

## Key Features

- **Operations**: Insert, Update, Delete, Search, and Reorganize
- **Buffering**: Page buffer implementation for improved I/O performance
- **Configurable Parameters**:
  - Page size (blocking factor)
  - Buffer size
  - Fill factor (α)
  - Overflow area size factor (β)
  - Reorganization threshold (γ)