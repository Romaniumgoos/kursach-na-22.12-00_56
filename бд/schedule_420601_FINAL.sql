-- Расписание группы 420601 (groupid=1)
-- Маппинг предметов: 1=АПЭЦ, 2=ТВиМС, 3=ФизК, 4=ТГ, 5=ВМиКА, 6=БЖЧ, 7=ООП, 8=ОИнфБ, 9=БД, 10=МСиСвИТ, 11=Инф.час, 12=К.Ч.

BEGIN TRANSACTION;

-- ===== ПОНЕДЕЛЬНИК (weekday=0) =====
-- 1 пара: ВМиКА (ЛР) - 1 п. неделя 3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 1, 0, 1, 3, 5, 119, '601б-5', 'ЛР');
-- 1 пара: ВМиКА (ЛР) - 2 п. неделя 1
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 2, 0, 1, 1, 5, 119, '601б-5', 'ЛР');
-- 2 пара: ТГ (ЛК) - недели 1-4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 0, 2, 1, 4, 122, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 0, 2, 2, 4, 122, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 0, 2, 3, 4, 122, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 0, 2, 4, 4, 122, '104-4', 'ЛК');
-- 3 пара: ФизК (ПЗ) - недели 1-4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 0, 3, 1, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 0, 3, 2, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 0, 3, 3, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 0, 3, 4, 3, 139, 'спортзал', 'ПЗ');
-- 4 пара: МСиСвИТ (ЛК) - недели 2,3,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 0, 4, 2, 10, 124, '104-3', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 0, 4, 3, 10, 124, '104-3', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 0, 4, 4, 10, 124, '104-3', 'ЛК');
-- 4 пара: ТВиМС (ЛК) - неделя 1
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 0, 4, 1, 2, 125, '108-4', 'ЛК');
-- 5 пара: БЖЧ (ЛР) - 1 п. неделя 1
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 1, 0, 5, 1, 6, 127, '603-2', 'ЛР');
-- 5 пара: БЖЧ (ЛР) - 2 п. неделя 3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 2, 0, 5, 3, 6, 127, '603-2', 'ЛР');
-- 5 пара: БЖЧ (ПЗ) - недели 2,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 0, 5, 2, 6, 127, '603-2', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 0, 5, 4, 6, 127, '603-2', 'ПЗ');

-- ===== ВТОРНИК (weekday=1) =====
-- 1 пара: ВМиКА (ЛК) - недели 1-4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 1, 1, 1, 5, 118, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 1, 1, 2, 5, 118, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 1, 1, 3, 5, 118, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 1, 1, 4, 5, 118, '414-5', 'ЛК');
-- 2 пара: АПЭЦ (ЛК) - недели 1-4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 1, 2, 1, 1, 120, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 1, 2, 2, 1, 120, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 1, 2, 3, 1, 120, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 1, 2, 4, 1, 120, '414-5', 'ЛК');
-- 3 пара: АПЭЦ (ЛР) - 1 п. недели 2,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 1, 1, 3, 2, 1, 120, '512-4', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 1, 1, 3, 4, 1, 120, '512-4', 'ЛР');
-- 3 пара: МСиСвИТ (ПЗ) - недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 1, 3, 1, 10, 124, '503-3', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 1, 3, 3, 10, 124, '503-3', 'ПЗ');
-- 3 пара: ООП (ЛР) - 2 п. недели 2,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 2, 1, 3, 2, 7, 130, '604-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 2, 1, 3, 4, 7, 130, '604-5', 'ЛР');
-- 4 пара: ООП (ЛР) - 1 п. недели 2,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 1, 1, 4, 2, 7, 130, '604-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 1, 1, 4, 4, 7, 130, '604-5', 'ЛР');
-- 4 пара: АПЭЦ (ЛР) - 2 п. недели 2,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 2, 1, 4, 2, 1, 120, '512-4', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 2, 1, 4, 4, 1, 120, '512-4', 'ЛР');

-- ===== СРЕДА (weekday=2) =====
-- 1 пара: Инф. час (ПЗ) - недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 1, 1, 11, 136, '605-5', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 1, 3, 11, 136, '605-5', 'ПЗ');
-- 1 пара: К.Ч. (ПЗ) - недели 2,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 1, 2, 12, 136, '605-5', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 1, 4, 12, 136, '605-5', 'ПЗ');
-- 2 пара: ООП (ЛК) - недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 2, 1, 7, 131, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 2, 3, 7, 131, '106-4', 'ЛК');
-- 2 пара: ТВиМС (ЛК) - недели 2,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 2, 2, 2, 125, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 2, 4, 2, 125, '106-4', 'ЛК');
-- 3 пара: БЖЧ (ЛК) - недели 1-4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 3, 1, 6, 127, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 3, 2, 6, 127, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 3, 3, 6, 127, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 3, 4, 6, 127, '106-4', 'ЛК');
-- 4 пара: ФизК (ПЗ) - недели 1-4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 4, 1, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 4, 2, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 4, 3, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 4, 4, 3, 139, 'спортзал', 'ПЗ');
-- 5 пара: АПЭЦ (ЛР) - неделя 1
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 5, 1, 1, 120, '512-4', 'ЛР');
-- 5 пара: БД (ПЗ) - неделя 3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 5, 3, 9, 135, '605-5', 'ПЗ');
-- 5 пара: БЖЧ (ЛР) - неделя 2
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 5, 2, 6, 127, '603-2', 'ЛР');
-- 5 пара: МСиСвИТ (ПЗ) - неделя 4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 2, 5, 4, 10, 124, '503-3', 'ПЗ');

-- ===== ЧЕТВЕРГ (weekday=3) =====
-- 1 пара: ОИнфБ (ЛК) - недели 1-4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 3, 1, 1, 8, 132, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 3, 1, 2, 8, 132, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 3, 1, 3, 8, 132, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 3, 1, 4, 8, 132, '104-4', 'ЛК');
-- 2 пара: ООП (ЛК) - недели 1-4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 3, 2, 1, 7, 131, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 3, 2, 2, 7, 131, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 3, 2, 3, 7, 131, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 3, 2, 4, 7, 131, '104-4', 'ЛК');
-- 3 пара: ВМиКА (ЛР) - неделя 4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 3, 3, 4, 5, 119, '601б-5', 'ЛР');
-- 3 пара: ООП (ЛР) - недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 3, 3, 1, 7, 130, '601б-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 3, 3, 3, 7, 130, '601б-5', 'ЛР');
-- 3 пара: ТВиМС (ПЗ) - неделя 2
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 3, 3, 2, 2, 126, '411-5', 'ПЗ');
-- 4 пара: БД (ЛР) - 1 п. недели 2,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 1, 3, 4, 2, 9, 134, '605-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 1, 3, 4, 4, 9, 134, '605-5', 'ЛР');
-- 4 пара: ТГ (ПЗ) - недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 3, 4, 1, 4, 123, '601б-5', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 3, 4, 3, 4, 123, '601б-5', 'ПЗ');

-- ===== ПЯТНИЦА (weekday=4) =====
-- 3 пара: БД (ЛР) - 2 п. недели 2,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 2, 4, 3, 2, 9, 134, '605-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 2, 4, 3, 4, 9, 134, '605-5', 'ЛР');
-- 4 пара: БД (ЛР) - неделя 2
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 4, 4, 2, 9, 134, '605-5', 'ЛР');
-- 4 пара: ТВиМС (ПЗ) - неделя 4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 4, 4, 4, 2, 126, '411-5', 'ПЗ');
-- 5 пара: ОИнфБ (ПЗ) - недели 2,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 4, 5, 2, 8, 133, '607-5', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 4, 5, 4, 8, 133, '607-5', 'ПЗ');

-- ===== СУББОТА (weekday=5) =====
-- 2 пара: ТВиМС (ПЗ) - неделя 1
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 5, 2, 1, 2, 126, '411-5', 'ПЗ');
-- 3 пара: БД (ЛК) - недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 5, 3, 1, 9, 134, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 5, 3, 3, 9, 134, '104-4', 'ЛК');
-- 4 пара: БД (ЛК) - недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 5, 4, 1, 9, 134, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (1, 0, 5, 4, 3, 9, 134, '104-4', 'ЛК');

COMMIT;
