-- Расписание группы 420604 (group_id=4)
-- Лекции (ЛК) вынесены в общее расписание (group_id=0)
-- Оставлены только практические занятия (ПЗ) и лабораторные работы (ЛР)
-- Без BEGIN/COMMIT - приложение управляет транзакциями

-- ===== ПОНЕДЕЛЬНИК (weekday=0) =====
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 1, 0, 1, 1, 1, 121, '512-4', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 1, 0, 1, 3, 1, 121, '512-4', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 2, 0, 1, 2, 1, 121, '512-4', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 2, 0, 1, 4, 1, 121, '512-4', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 0, 3, 1, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 0, 3, 2, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 0, 3, 3, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 0, 3, 4, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 0, 5, 1, 11, 138, '605-5', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 0, 5, 3, 11, 138, '605-5', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 0, 5, 2, 12, 138, '605-5', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 0, 5, 4, 12, 138, '605-5', 'ПЗ');

-- ===== ВТОРНИК (weekday=1) =====
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 1, 3, 2, 5, 119, '601б-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 1, 1, 3, 4, 5, 119, '601б-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 1, 3, 1, 4, 123, '601б-5', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 1, 3, 3, 4, 123, '601б-5', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 2, 1, 4, 2, 5, 119, '601б-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 1, 4, 3, 2, 126, '411-5', 'ПЗ');

-- ===== СРЕДА (weekday=2) =====
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 2, 4, 1, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 2, 4, 2, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 2, 4, 3, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 2, 4, 4, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 2, 5, 4, 9, 135, '605-5', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 2, 5, 2, 10, 124, '503-3', 'ПЗ');


-- ===== ЧЕТВЕРГ (weekday=3) =====
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 3, 3, 1, 6, 128, '603-2', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 3, 3, 3, 6, 128, '603-2', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 3, 3, 2, 10, 124, '503-3', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 3, 3, 4, 10, 124, '503-3', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 1, 3, 4, 1, 6, 128, '603-2', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 2, 3, 4, 3, 6, 128, '603-2', 'ЛР');

-- ===== ПЯТНИЦА (weekday=4) =====
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 1, 4, 1, 2, 9, 134, '601а-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 1, 4, 1, 4, 9, 134, '601а-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 1, 4, 1, 1, 7, 130, '601б-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 1, 4, 1, 3, 7, 130, '601б-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 2, 4, 1, 1, 9, 134, '601а-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 2, 4, 1, 3, 9, 134, '601а-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 2, 4, 1, 2, 7, 130, '601б-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 2, 4, 1, 4, 7, 130, '601б-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 4, 2, 1, 7, 130, '601б-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 4, 2, 3, 7, 130, '601б-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 4, 2, 4, 2, 126, '411-5', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 4, 3, 3, 1, 121, '512-4', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 4, 3, 2, 8, 133, '508-3', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 4, 3, 4, 8, 133, '612а-5', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 4, 3, 1, 2, 126, '411-5', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (4, 0, 4, 4, 2, 6, 128, '603-2', 'ЛР');

-- ===== СУББОТА (weekday=5) =====
