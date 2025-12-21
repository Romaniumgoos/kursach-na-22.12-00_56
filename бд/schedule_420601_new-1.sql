-- Расписание группы 420601 (groupid=1)
-- Маппинг ID преподавателей:
-- 118=Батин (ВМиКА ЛК), 119=Боброва (ВМиКА ЛР), 120=Шилин (АПЭЦ ЛК), 
-- 122=Севернёв (ТГ ЛК), 124=Дерябина (МСиСвИТ), 125=Гуревич (ТВиМС ЛК), 
-- 127=Рышкель (ФизК), 129=Пригара (АПЭЦ ЛР), 130=Ючков (ООП ЛР), 131=Герман (ООП ЛК), 
-- 136=Езовит (Инф. час), 137=Трофимович (К.Ч.), 139=Физкультуров (ФизК)

BEGIN TRANSACTION;

-- ===== ПОНЕДЕЛЬНИК (weekday=0) =====
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 1, 1, 1, 118, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 1, 2, 1, 118, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 1, 3, 1, 118, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 1, 4, 1, 118, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 2, 1, 2, 120, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 2, 2, 2, 120, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 2, 3, 2, 120, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 2, 4, 2, 120, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 3, 1, 3, 127, 'спортзал');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 3, 2, 3, 127, 'спортзал');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 3, 3, 3, 127, 'спортзал');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 3, 4, 3, 127, 'спортзал');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 4, 2, 4, 126, '104-3');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 4, 3, 4, 126, '104-3');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 4, 4, 4, 126, '104-3');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 4, 1, 5, 124, '108-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 5, 1, 5, 125, '603-2');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 5, 2, 5, 125, '603-2');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 5, 3, 5, 125, '603-2');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 0, 5, 4, 5, 125, '603-2');

-- ===== ВТОРНИК (weekday=1) =====
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 1, 1, 1, 1, 118, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 1, 1, 2, 1, 118, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 1, 1, 3, 1, 118, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 1, 1, 4, 1, 118, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 1, 2, 1, 2, 120, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 1, 2, 2, 2, 120, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 1, 2, 3, 2, 120, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 1, 2, 4, 2, 120, '414-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 1, 3, 1, 3, 127, '503-3');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 1, 3, 3, 3, 127, '503-3');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 1, 3, 2, 2, 130, '604-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 1, 3, 4, 2, 130, '604-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 1, 4, 2, 2, 130, '604-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 1, 4, 4, 2, 130, '604-5');

-- ===== СРЕДА (weekday=2) =====
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 1, 1, 9, 136, '605-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 1, 3, 9, 136, '605-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 1, 2, 11, 119, '605-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 1, 4, 11, 119, '605-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 2, 1, 2, 130, '106-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 2, 3, 2, 130, '106-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 2, 2, 5, 124, '106-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 2, 4, 5, 124, '106-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 3, 1, 5, 125, '106-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 3, 2, 5, 125, '106-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 3, 3, 5, 125, '106-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 3, 4, 5, 125, '106-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 4, 1, 3, 127, 'спортзал');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 4, 2, 3, 127, 'спортзал');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 4, 3, 3, 127, 'спортзал');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 4, 4, 3, 127, 'спортзал');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 5, 1, 2, 120, '512-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 5, 3, 12, 137, '605-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 5, 2, 5, 125, '603-2');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 2, 5, 4, 4, 126, '503-3');

-- ===== ЧЕТВЕРГ (weekday=3) =====
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 1, 1, 9, 136, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 1, 2, 9, 136, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 1, 3, 9, 136, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 1, 4, 9, 136, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 2, 1, 2, 130, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 2, 2, 2, 130, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 2, 3, 2, 130, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 2, 4, 2, 130, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 3, 1, 2, 130, '601б-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 3, 3, 2, 130, '601б-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 3, 4, 1, 118, '601б-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 3, 2, 5, 125, '411-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 4, 2, 12, 137, '605-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 4, 4, 12, 137, '605-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 4, 1, 2, 120, '601б-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 3, 4, 3, 2, 120, '601б-5');

-- ===== ПЯТНИЦА (weekday=4) =====
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 4, 3, 2, 12, 137, '605-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 4, 3, 4, 12, 137, '605-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 4, 4, 2, 12, 137, '605-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 4, 4, 4, 5, 125, '411-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 4, 5, 2, 9, 136, '607-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 4, 5, 4, 9, 136, '607-5');

-- ===== СУББОТА (weekday=5) =====
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 5, 2, 1, 5, 125, '411-5');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 5, 3, 1, 11, 137, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 5, 3, 3, 11, 137, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 5, 4, 1, 11, 137, '104-4');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room) VALUES (1, 0, 5, 4, 3, 11, 137, '104-4');

COMMIT;
