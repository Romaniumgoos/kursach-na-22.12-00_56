-- Расписание группы 420603 (groupid=3) - ПОЛНОЕ НА ВСЕ НЕДЕЛИ ЦИКЛА
-- Структура: groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room

BEGIN TRANSACTION;

-- ===== ПОНЕДЕЛЬНИК (weekday=0) =====
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 0, 1, 1, 1, 123, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 0, 1, 2, 1, 123, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 0, 1, 3, 1, 123, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 0, 1, 4, 1, 123, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 0, 2, 1, 2, 124, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 0, 2, 2, 2, 124, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 0, 2, 3, 2, 124, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 0, 2, 4, 2, 124, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 0, 3, 1, 3, 122, 'спортзал');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 0, 3, 2, 3, 122, 'спортзал');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 0, 3, 3, 3, 122, 'спортзал');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 0, 3, 4, 3, 122, 'спортзал');

-- ===== ВТОРНИК (weekday=1) =====
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 1, 1, 1, 1, 123, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 1, 1, 2, 1, 123, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 1, 1, 3, 1, 123, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 1, 1, 4, 1, 123, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 1, 2, 1, 2, 124, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 1, 2, 2, 2, 124, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 1, 2, 3, 2, 124, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 1, 2, 4, 2, 124, '414-5');

-- ===== СРЕДА (weekday=2) =====
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 2, 1, 1, 9, 125, '605-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 2, 1, 2, 9, 125, '605-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 2, 2, 1, 2, 126, '106-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 2, 2, 2, 2, 126, '106-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 2, 3, 1, 5, 128, '106-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 2, 3, 2, 5, 128, '106-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 2, 4, 1, 3, 122, 'спортзал');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 2, 4, 2, 3, 122, 'спортзал');

-- ===== ЧЕТВЕРГ (weekday=3) =====
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 3, 1, 1, 9, 125, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 3, 1, 2, 9, 125, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 3, 2, 1, 2, 126, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 3, 2, 2, 2, 126, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 3, 3, 1, 2, 126, '601б-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 3, 3, 2, 2, 126, '601б-5');

-- ===== ПЯТНИЦА (weekday=4) =====
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 4, 3, 1, 12, 134, '605-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 4, 3, 2, 12, 134, '605-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 4, 5, 2, 9, 131, '607-5');

-- ===== СУББОТА (weekday=5) =====
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 5, 3, 1, 11, 134, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (3, 0, 5, 3, 2, 11, 134, '104-4');

COMMIT;
