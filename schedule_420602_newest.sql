-- Расписание группы 420602 (group_id=2)
-- Лекции (ЛК) вынесены в общее расписание (group_id=0)
-- Оставлены только практические занятия (ПЗ) и лабораторные работы (ЛР)
-- Без BEGIN/COMMIT - приложение управляет транзакциями

-- ===== ПОНЕДЕЛЬНИК (weekday=0) =====
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 0, 3, 1, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 0, 3, 2, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 0, 3, 3, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 0, 3, 4, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 0, 5, 2, 10, 124, '503-3', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 0, 5, 3, 10, 124, '503-3', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 0, 5, 4, 10, 124, '503-3', 'ПЗ');

-- ===== ВТОРНИК (weekday=1) =====

-- ===== СРЕДА (weekday=2) =====
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 2, 1, 2, 1, 129, '510-4', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 2, 1, 1, 11, 130, '423-4', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 2, 1, 3, 11, 130, '423-4', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 2, 1, 4, 12, 130, '423-4', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 2, 4, 1, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 2, 4, 2, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 2, 4, 3, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 2, 4, 4, 3, 139, 'спортзал', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 2, 5, 1, 9, 135, '605-5', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 2, 5, 4, 6, 127, '603-2', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 2, 5, 2, 12, 130, '423-4', 'ПЗ');

-- ===== ЧЕТВЕРГ (weekday=3) =====
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 3, 3, 2, 5, 119, '601б-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 3, 3, 1, 2, 126, '411-5', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 3, 3, 3, 2, 126, '411-5', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 1, 3, 4, 2, 5, 119, '601б-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 1, 3, 4, 1, 7, 130, '605-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 1, 3, 4, 3, 7, 130, '605-5', 'ЛР');

-- ===== ПЯТНИЦА (weekday=4) =====
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 1, 4, 2, 1, 9, 134, '601а-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 1, 4, 2, 3, 9, 134, '601а-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 2, 4, 2, 1, 1, 129, '512-4', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 2, 4, 2, 3, 1, 129, '512-4', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 4, 2, 2, 7, 130, '423-4', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 4, 2, 4, 7, 130, '601б-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 4, 3, 1, 9, 134, '605-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 4, 3, 2, 6, 127, '605-2', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 4, 3, 4, 6, 127, '605-2', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 4, 3, 3, 2, 126, '411-5', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 1, 4, 4, 1, 1, 129, '512-4', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 1, 4, 4, 3, 1, 129, '512-4', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 2, 4, 4, 4, 6, 127, '605-2', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 1, 4, 4, 2, 6, 127, '605-2', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 2, 4, 4, 1, 9, 134, '605-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 2, 4, 4, 3, 9, 134, '605-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 2, 4, 5, 4, 5, 119, '601б-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 4, 5, 1, 8, 133, '607-5', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 4, 5, 3, 8, 133, '607-5', 'ПЗ');

-- ===== СУББОТА (weekday=5) =====
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 2, 5, 1, 1, 7, 130, '605-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 2, 5, 1, 3, 7, 130, '605-5', 'ЛР');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 5, 2, 1, 4, 123, '601б-5', 'ПЗ');
INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type) VALUES (2, 0, 5, 2, 3, 4, 123, '601б-5', 'ПЗ');
