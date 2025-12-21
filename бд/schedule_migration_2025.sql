BEGIN TRANSACTION;

DROP TABLE IF EXISTS schedule;

CREATE TABLE schedule (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    groupid      INTEGER NOT NULL,
    subgroup     INTEGER NOT NULL,
    weekday      INTEGER NOT NULL,
    lessonnumber INTEGER NOT NULL,
    weekofcycle  INTEGER NOT NULL CHECK(weekofcycle BETWEEN 1 AND 4),
    subjectid    INTEGER NOT NULL,
    teacherid    INTEGER NOT NULL,
    room         TEXT,
    FOREIGN KEY (groupid)   REFERENCES groups(id),
    FOREIGN KEY (subjectid) REFERENCES subjects(id),
    FOREIGN KEY (teacherid) REFERENCES users(id)
);

CREATE INDEX idx_schedule_groupweek
    ON schedule (groupid, weekday, weekofcycle);

CREATE INDEX idx_schedule_teacher
    ON schedule (teacherid);

CREATE INDEX idx_schedule_room
    ON schedule (room);

COMMIT;
