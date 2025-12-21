-- Расписание группы 420603 (groupid=3)
-- Без BEGIN/COMMIT - приложение управляет транзакциями

-- ===== ПОНЕДЕЛЬНИК (weekday=0) =====

-- 1 пара: ООП (ЛР) подгруппа 2 недели 2,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 2, 0, 1, 2, 6, 134, '601б-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 2, 0, 1, 4, 6, 134, '601б-5', 'ЛР');

-- 2 пара: ТГ (ЛК) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 0, 2, 1, 4, 122, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 0, 2, 2, 4, 122, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 0, 2, 3, 4, 122, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 0, 2, 4, 4, 122, '104-4', 'ЛК');

-- 3 пара: ФизК (ПЗ) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 0, 3, 1, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 0, 3, 2, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 0, 3, 3, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 0, 3, 4, 3, 139, 'спортзал', 'ПЗ');

-- 4 пара: МСиСвИТ (ЛК) недели 2,3,4 + ТВиМС (ЛК) неделя 1
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 0, 4, 1, 2, 125, '108-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 0, 4, 2, 10, 124, '104-3', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 0, 4, 3, 10, 124, '104-3', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 0, 4, 4, 10, 124, '104-3', 'ЛК');

-- 5 пара: МСиСвИТ (ПЗ) неделя 1
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 0, 5, 1, 10, 124, '503-3', 'ПЗ');

-- ===== ВТОРНИК (weekday=1) =====

-- 1 пара: ВМиКА (ЛК) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 1, 1, 1, 5, 118, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 1, 1, 2, 5, 118, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 1, 1, 3, 5, 118, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 1, 1, 4, 5, 118, '414-5', 'ЛК');

-- 2 пара: АПЭЦ (ЛК) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 1, 2, 1, 1, 120, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 1, 2, 2, 1, 120, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 1, 2, 3, 1, 120, '414-5', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 1, 2, 4, 1, 120, '414-5', 'ЛК');

-- 3 пара: ТВиМС (ПЗ) недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 1, 3, 1, 2, 126, '411-5', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 1, 3, 3, 2, 126, '411-5', 'ПЗ');

-- 4 пара: ООП (ЛР) подгруппа 1 недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 1, 1, 4, 1, 6, 134, '604-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 1, 1, 4, 3, 6, 134, '604-5', 'ЛР');

-- ===== СРЕДА (weekday=2) =====

-- 1 пара: ОИнфБ (ПЗ) неделя 1
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 1, 1, 13, 137, '607-5', 'ПЗ');

-- 2 пара: ООП (ЛК) недели 1,3 + ТВиМС (ЛК) недели 2,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 2, 1, 6, 116, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 2, 3, 6, 116, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 2, 2, 2, 125, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 2, 4, 2, 125, '106-4', 'ЛК');

-- 3 пара: БЖЧ (ЛК) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 3, 1, 8, 129, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 3, 2, 8, 129, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 3, 3, 8, 129, '106-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 3, 4, 8, 129, '106-4', 'ЛК');

-- 4 пара: ФизК (ПЗ) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 4, 1, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 4, 2, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 4, 3, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 4, 4, 3, 139, 'спортзал', 'ПЗ');

-- 5 пара: БД (ПЗ) неделя 2 + Инф. час (ПЗ) недели 1,3 + ОИнфБ (ПЗ) неделя 4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 5, 2, 7, 130, '605-5', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 5, 1, 11, 133, '601а-5', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 5, 3, 11, 133, '601а-5', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 2, 5, 4, 13, 137, '316-4', 'ПЗ');

-- ===== ЧЕТВЕРГ (weekday=3) =====

-- 1 пара: ОИнфБ (ЛК) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 3, 1, 1, 13, 135, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 3, 1, 2, 13, 135, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 3, 1, 3, 13, 135, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 3, 1, 4, 13, 135, '104-4', 'ЛК');

-- 2 пара: ООП (ЛК) - все недели, общая
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 3, 2, 1, 6, 116, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 3, 2, 2, 6, 116, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 3, 2, 3, 6, 116, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 3, 2, 4, 6, 116, '104-4', 'ЛК');

-- 3 пара: АПЭЦ (ЛР) подгруппы + БД (ЛР) подгруппы
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 1, 3, 3, 1, 1, 141, '512-4', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 1, 3, 3, 3, 1, 141, '512-4', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 1, 3, 3, 2, 7, 131, '605-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 1, 3, 3, 4, 7, 131, '605-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 2, 3, 3, 2, 1, 141, '512-4', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 2, 3, 3, 4, 1, 141, '512-4', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 2, 3, 3, 1, 7, 131, '605-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 2, 3, 3, 3, 7, 131, '605-5', 'ЛР');

-- 4 пара: АПЭЦ (ЛР) общая неделя 2 + БД (ЛР) общая неделя 1
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 3, 4, 2, 1, 141, '512-4', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 3, 4, 1, 7, 131, '604-5', 'ЛР');

-- ===== ПЯТНИЦА (weekday=4) =====

-- 2 пара: БЖЧ (ЛР) общая неделя 2 + БЖЧ (ЛР) подгруппа 2 неделя 4 + БЖЧ (ПЗ) неделя 3 + ВМиКА (ЛР) неделя 2 + МСиСвИТ (ПЗ) недели 1,3 + ТВиМС (ПЗ) неделя 4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 4, 2, 2, 8, 142, '603-2', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 2, 4, 2, 4, 8, 142, '603-2', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 4, 2, 3, 8, 129, '603-2', 'ПЗ');

-- 3 пара: ВМиКА (ЛР) неделя 2 + МСиСвИТ (ПЗ) недели 1,3 + ТВиМС (ПЗ) неделя 4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 4, 3, 2, 5, 117, '601б-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 4, 3, 1, 10, 124, '503-3', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 4, 3, 3, 10, 124, '503-3', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 4, 3, 4, 2, 126, '411-5', 'ПЗ');

-- 4 пара: БЖЧ (ЛР) неделя 3 + ВМиКА (ЛР) подгруппы
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 4, 4, 3, 8, 142, '502-2', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 1, 4, 4, 4, 5, 117, '601б-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 2, 4, 4, 2, 5, 117, '605-5', 'ЛР');

-- 5 пара: К.Ч. (ПЗ) недели 2,4
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 4, 5, 2, 14, 133, '605-5', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 4, 5, 4, 14, 133, '605-5', 'ПЗ');

-- ===== СУББОТА (weekday=5) =====

-- 1 пара: ТГ (ПЗ) недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 5, 1, 1, 4, 136, '601б-5', 'ПЗ');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 5, 1, 3, 4, 136, '601б-5', 'ПЗ');

-- 2 пара: ООП (ЛР) недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 5, 2, 1, 6, 134, '605-5', 'ЛР');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 5, 2, 3, 6, 134, '605-5', 'ЛР');

-- 3 пара: БД (ЛК) недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 5, 3, 1, 7, 131, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 5, 3, 3, 7, 131, '104-4', 'ЛК');

-- 4 пара: БД (ЛК) недели 1,3
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 5, 4, 1, 7, 131, '104-4', 'ЛК');
INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lesson_type) VALUES (3, 0, 5, 4, 3, 7, 131, '104-4', 'ЛК');