-- Расписание группы 420602 (groupid=2)
-- Без BEGIN/COMMIT - приложение управляет транзакциями

-- ===== ПОНЕДЕЛЬНИК (weekday=0) =====

-- 2 пара: ТГ (ЛК) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 0, 2, 1, 4, 122, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 0, 2, 2, 4, 122, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 0, 2, 3, 4, 122, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 0, 2, 4, 4, 122, '104-4', 'ЛК');

-- 3 пара: ФизК (ПЗ) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 0, 3, 1, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 0, 3, 2, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 0, 3, 3, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 0, 3, 4, 3, 139, 'спортзал', 'ПЗ');

-- 4 пара: МСиСвИТ (ЛК) недели 2,3,4 + ТВиМС (ЛК) неделя 1
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 0, 4, 1, 2, 125, '108-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 0, 4, 2, 10, 124, '104-3', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 0, 4, 3, 10, 124, '104-3', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 0, 4, 4, 10, 124, '104-3', 'ЛК');

-- 5 пара: МСиСвИТ (ПЗ) недели 2,3,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 0, 5, 2, 10, 124, '503-3', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 0, 5, 3, 10, 124, '503-3', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 0, 5, 4, 10, 124, '503-3', 'ПЗ');

-- ===== ВТОРНИК (weekday=1) =====

-- 1 пара: ВМиКА (ЛК) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 1, 1, 1, 5, 118, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 1, 1, 2, 5, 118, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 1, 1, 3, 5, 118, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 1, 1, 4, 5, 118, '414-5', 'ЛК');

-- 2 пара: АПЭЦ (ЛК) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 1, 2, 1, 1, 120, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 1, 2, 2, 1, 120, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 1, 2, 3, 1, 120, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 1, 2, 4, 1, 120, '414-5', 'ЛК');

-- ===== СРЕДА (weekday=2) =====

-- 1 пара: АПЭЦ (ЛР) неделя 2 + Инф. час (ПЗ) недели 1,3 + К.Ч. (ПЗ) неделя 4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 1, 2, 1, 140, '510-4', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 1, 1, 11, 134, '423-4', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 1, 3, 11, 134, '423-4', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 1, 4, 14, 134, '423-4', 'ПЗ');

-- 2 пара: ООП (ЛК) недели 1,3 + ТВиМС (ЛК) недели 2,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 2, 1, 6, 116, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 2, 3, 6, 116, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 2, 2, 2, 125, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 2, 4, 2, 125, '106-4', 'ЛК');

-- 3 пара: БЖЧ (ЛК) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 3, 1, 8, 129, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 3, 2, 8, 129, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 3, 3, 8, 129, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 3, 4, 8, 129, '106-4', 'ЛК');

-- 4 пара: ФизК (ПЗ) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 4, 1, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 4, 2, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 4, 3, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 4, 4, 3, 139, 'спортзал', 'ПЗ');

-- 5 пара: БД (ПЗ) неделя 1 + К.Ч. (ПЗ) неделя 2 + БЖЧ (ЛР) неделя 4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 5, 1, 7, 130, '605-5', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 5, 2, 14, 134, '423-4', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 2, 5, 4, 8, 129, '603-2', 'ЛР');

-- ===== ЧЕТВЕРГ (weekday=3) =====

-- 1 пара: ОИнфБ (ЛК) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 3, 1, 1, 13, 135, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 3, 1, 2, 13, 135, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 3, 1, 3, 13, 135, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 3, 1, 4, 13, 135, '104-4', 'ЛК');

-- 2 пара: ООП (ЛК) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 3, 2, 1, 6, 116, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 3, 2, 2, 6, 116, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 3, 2, 3, 6, 116, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 3, 2, 4, 6, 116, '104-4', 'ЛК');

-- 3 пара: ВМиКА (ЛР) неделя 2 + ТВиМС (ПЗ) недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 3, 3, 2, 5, 117, '601б-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 3, 3, 1, 2, 126, '411-5', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 3, 3, 3, 2, 126, '411-5', 'ПЗ');

-- 4 пара: ВМиКА (ЛР) подгруппы + ООП (ЛР) подгруппа 1
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 1, 3, 4, 2, 5, 117, '601б-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 1, 3, 4, 1, 6, 134, '604-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 1, 3, 4, 3, 6, 134, '604-5', 'ЛР');

-- ===== ПЯТНИЦА (weekday=4) =====

-- 2 пара: БД (ЛР) подгруппа 1 недели 1,3 + АПЭЦ (ЛР) подгруппа 2 недели 1,3 + ООП (ЛР) общая неделя 2 + ООП (ЛР) общая неделя 4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 1, 4, 2, 1, 7, 131, '601а-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 1, 4, 2, 3, 7, 131, '601а-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 2, 4, 2, 1, 1, 140, '512-4', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 2, 4, 2, 3, 1, 140, '512-4', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 4, 2, 2, 6, 134, '423-4', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 4, 2, 4, 6, 134, '601б-5', 'ЛР');

-- 3 пара: БД (ЛР) общая неделя 1 + БЖЧ (ПЗ) недели 2,4 + ТВиМС (ПЗ) неделя 3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 4, 3, 1, 7, 131, '605-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 4, 3, 2, 8, 129, '605-2', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 4, 3, 4, 8, 129, '605-2', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 4, 3, 3, 2, 126, '411-5', 'ПЗ');

-- 4 пара: АПЭЦ (ЛР) подгруппа 1 недели 1,3 + БЖЧ (ЛР) подгруппа 2 недели 1,3 + БД (ЛР) подгруппа 2 недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 1, 4, 4, 1, 1, 140, '512-4', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 1, 4, 4, 3, 1, 140, '512-4', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 2, 4, 4, 1, 8, 129, '605-2', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 2, 4, 4, 3, 8, 129, '605-2', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 2, 4, 4, 1, 7, 131, '605-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 2, 4, 4, 3, 7, 131, '605-5', 'ЛР');

-- 5 пара: ВМиКА (ЛР) подгруппа 2 неделя 4 + ОИнфБ (ПЗ) недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 2, 4, 5, 4, 5, 117, '601б-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 4, 5, 1, 13, 137, '607-5', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 4, 5, 3, 13, 137, '607-5', 'ПЗ');

-- ===== СУББОТА (weekday=5) =====

-- 1 пара: ООП (ЛР) подгруппа 2 недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 2, 5, 1, 1, 6, 134, '605-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 2, 5, 1, 3, 6, 134, '605-5', 'ЛР');

-- 2 пара: ТГ (ПЗ) недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 5, 2, 1, 4, 136, '601б-5', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 5, 2, 3, 4, 136, '601б-5', 'ПЗ');

-- 3 пара: БД (ЛК) недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 5, 3, 1, 7, 131, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 5, 3, 3, 7, 131, '104-4', 'ЛК');

-- 4 пара: БД (ЛК) недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 5, 4, 1, 7, 131, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (2, 0, 5, 4, 3, 7, 131, '104-4', 'ЛК');