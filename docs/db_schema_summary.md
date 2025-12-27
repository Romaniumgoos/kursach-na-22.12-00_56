# DB schema summary (from code / SQL)

Источник: `Database::initialize()` в `database.cpp` (строка с `CREATE TABLE IF NOT EXISTS ...`).

Также в проекте присутствуют SQL-скрипты загрузки расписаний:
- `schedule_420601_newest.sql`
- `schedule_420602_newest.sql`
- `schedule_420603_newest.sql`
- `schedule_420604_newest.sql`

## 1) Таблицы

### `groups`
Назначение: учебные группы.
- `id` INTEGER PK AUTOINCREMENT
- `name` TEXT UNIQUE NOT NULL

Связи:
- `users.groupid` -> `groups.id`
- `teachergroups.groupid` -> `groups.id`
- `schedule.groupid` -> `groups.id`

### `semesters`
Назначение: семестры/сессии.
- `id` INTEGER PK AUTOINCREMENT
- `name` TEXT UNIQUE NOT NULL
- `startdate` TEXT
- `enddate` TEXT

Связи:
- `grades.semesterid` -> `semesters.id`
- `absences.semesterid` -> `semesters.id`

### `users`
Назначение: пользователи системы (все роли).
- `id` INTEGER PK AUTOINCREMENT
- `username` TEXT UNIQUE NOT NULL
- `password` TEXT NOT NULL
- `role` TEXT NOT NULL CHECK(role IN ('admin','teacher','student'))
- `name` TEXT NOT NULL
- `groupid` INTEGER NULLABLE
- `subgroup` INTEGER DEFAULT 0

Связи:
- `users.groupid` -> `groups.id`

Примечание:
- Для `student` ожидается `groupid` и `subgroup`.
- Для `admin/teacher` допускается `groupid=0` или NULL (по смыслу комментариев в `database.h`).

### `subjects`
Назначение: учебные предметы.
- `id` INTEGER PK AUTOINCREMENT
- `name` TEXT UNIQUE NOT NULL

Связи:
- `teachersubjects.subjectid` -> `subjects.id`
- `schedule.subjectid` -> `subjects.id`
- `grades.subjectid` -> `subjects.id`
- `absences.subjectid` -> `subjects.id`

### `teachersubjects`
Назначение: связь преподаватель–предмет (many-to-many).
- `teacherid` INTEGER NOT NULL
- `subjectid` INTEGER NOT NULL
- PRIMARY KEY (`teacherid`, `subjectid`)

Связи:
- `teacherid` -> `users.id`
- `subjectid` -> `subjects.id`

### `teachergroups`
Назначение: связь преподаватель–группа (many-to-many).
- `teacherid` INTEGER NOT NULL
- `groupid` INTEGER NOT NULL
- PRIMARY KEY (`teacherid`, `groupid`)

Связи:
- `teacherid` -> `users.id`
- `groupid` -> `groups.id`

### `schedule`
Назначение: расписание (основа отображения для student/teacher/admin).
- `id` INTEGER PK AUTOINCREMENT
- `groupid` INTEGER NOT NULL
- `subgroup` INTEGER NOT NULL
- `weekday` INTEGER NOT NULL
- `lessonnumber` INTEGER NOT NULL
- `weekofcycle` INTEGER NOT NULL CHECK(1..4)
- `subjectid` INTEGER NOT NULL
- `teacherid` INTEGER NOT NULL
- `room` TEXT
- `lessontype` TEXT

Связи:
- `groupid` -> `groups.id`
- `subjectid` -> `subjects.id`
- `teacherid` -> `users.id`

Индексы:
- `idxschedulegroupweek` on (`groupid`, `weekday`, `weekofcycle`)
- `idxscheduleteacher` on (`teacherid`)
- `idxscheduleroom` on (`room`)

### `grades`
Назначение: оценки.
- `id` INTEGER PK AUTOINCREMENT
- `studentid` INTEGER NOT NULL
- `subjectid` INTEGER NOT NULL
- `semesterid` INTEGER NOT NULL
- `value` INTEGER NOT NULL CHECK(0..10)
- `date` TEXT
- `gradetype` TEXT

Связи:
- `studentid` -> `users.id`
- `subjectid` -> `subjects.id`
- `semesterid` -> `semesters.id`

### `gradechanges`
Назначение: история изменений оценок.
- `id` INTEGER PK AUTOINCREMENT
- `gradeid` INTEGER NOT NULL
- `oldvalue` INTEGER
- `newvalue` INTEGER
- `changedby` INTEGER NOT NULL
- `changedate` TEXT NOT NULL
- `comment` TEXT

Связи:
- `gradeid` -> `grades.id`
- `changedby` -> `users.id`

### `cycleweeks`
Назначение: календарь 4-недельного цикла.
- `id` INTEGER PK AUTOINCREMENT
- `weekofcycle` INTEGER NOT NULL CHECK(1..4)
- `startdate` TEXT NOT NULL
- `enddate` TEXT NOT NULL

Связи:
- используется для вычисления недели цикла/дат в UI и выборках расписания

### `absences`
Назначение: пропуски.
- `id` INTEGER PK AUTOINCREMENT
- `studentid` INTEGER NOT NULL
- `subjectid` INTEGER NOT NULL
- `semesterid` INTEGER NOT NULL
- `hours` INTEGER NOT NULL
- `date` TEXT
- `type` TEXT

Связи:
- `studentid` -> `users.id`
- `subjectid` -> `subjects.id`
- `semesterid` -> `semesters.id`

## 2) Основные связи (коротко)
- `groups` 1—N `users` (для студентов)
- `users(teacher)` N—M `subjects` через `teachersubjects`
- `users(teacher)` N—M `groups` через `teachergroups`
- `schedule` ссылается на `groups`, `subjects`, `users(teacher)`
- `grades/absences` ссылаются на `users(student)`, `subjects`, `semesters`
- `gradechanges` — аудит (grade + кто изменил)

## 3) Где брать данные для ER-диаграммы
- Таблицы/поля: блок `Database::initialize()`
- Кардинальности: по FK из `CREATE TABLE`
- Индексы: после `schedule` (idxschedulegroupweek / idxscheduleteacher / idxscheduleroom)
