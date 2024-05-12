CREATE DATABASE batchalign;
USE batchalign;
CREATE TABLE cache (
    id VARCHAR(40) DEFAULT (uuid()) PRIMARY KEY,
    name TEXT NOT NULL,
    status VARCHAR(30),
    payload TEXT
);
