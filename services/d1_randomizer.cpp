#include "d1_randomizer.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <iostream>
#include <unordered_set>

namespace {

struct StudentRow {
    int id = 0;
    std::string name;
    int subgroup = 0;
};

static bool execGetStudentsOfGroupWithSubgroup(sqlite3* rawDb, int groupId, std::vector<StudentRow>& out)
{
    out.clear();
    if (!rawDb || groupId <= 0) return false;

    const char* sql =
        "SELECT id, name, COALESCE(subgroup, 0) "
        "FROM users "
        "WHERE role = 'student' AND groupid = ? "
        "ORDER BY name;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(rawDb, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, groupId);

    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        StudentRow r;
        r.id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        r.name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        r.subgroup = sqlite3_column_int(stmt, 2);
        out.push_back(std::move(r));
    }

    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

static bool execGetSemesterDateRange(sqlite3* rawDb, int semesterId, std::string& outStartDate, std::string& outEndDate)
{
    outStartDate.clear();
    outEndDate.clear();
    if (!rawDb || semesterId <= 0) return false;

    const char* sql = "SELECT COALESCE(startdate, ''), COALESCE(enddate, '') FROM semesters WHERE id = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(rawDb, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, semesterId);

    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char* s = sqlite3_column_text(stmt, 0);
        const unsigned char* e = sqlite3_column_text(stmt, 1);
        outStartDate = s ? reinterpret_cast<const char*>(s) : "";
        outEndDate = e ? reinterpret_cast<const char*>(e) : "";
        sqlite3_finalize(stmt);
        return true;
    }

    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

static bool execBegin(sqlite3* rawDb)
{
    if (!rawDb) return false;
    char* err = nullptr;
    const int rc = sqlite3_exec(rawDb, "BEGIN TRANSACTION;", nullptr, nullptr, &err);
    if (err) sqlite3_free(err);
    return rc == SQLITE_OK;
}

static bool execCommit(sqlite3* rawDb)
{
    if (!rawDb) return false;
    char* err = nullptr;
    const int rc = sqlite3_exec(rawDb, "COMMIT;", nullptr, nullptr, &err);
    if (err) sqlite3_free(err);
    return rc == SQLITE_OK;
}

static void execRollback(sqlite3* rawDb)
{
    if (!rawDb) return;
    sqlite3_exec(rawDb, "ROLLBACK;", nullptr, nullptr, nullptr);
}

static std::vector<std::string> uniqueSorted(std::vector<std::string> v)
{
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
    return v;
}

static std::vector<std::string> uniqueSortedFilteredNonEmpty(std::vector<std::string> v)
{
    v.erase(std::remove_if(v.begin(), v.end(), [](const std::string& s) { return s.empty(); }), v.end());
    return uniqueSorted(std::move(v));
}

static std::string trimCopy(std::string s)
{
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

} 

D1Randomizer::D1Randomizer(Database& db)
    : db(db)
{
    const auto seed = static_cast<std::uint32_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    rng.seed(seed);
}

std::vector<GradeRule> D1Randomizer::rules() const
{
    std::vector<GradeRule> r;

    r.push_back({"АПЭЦ", "ЛР", false, 4, 4, 5, 9, GradeBias::None, std::nullopt});
    r.push_back({"ТВиМС", "ПЗ", false, 2, 2, 0, 10, GradeBias::Often9_10, std::nullopt});
    r.push_back({"ТГ", "ПЗ", false, 5, 5, 0, 10, GradeBias::Often10, std::nullopt});

    r.push_back({"ВМиКА", "ЛК", true, 2, 2, 5, 10, GradeBias::None, std::nullopt});
    r.push_back({"ВМиКА", "ЛР", false, 4, 4, 6, 10, GradeBias::OftenGE8, std::nullopt});

    r.push_back({"БЖЧ", "ПЗ", false, 3, 5, 5, 10, GradeBias::None, std::nullopt});
    r.push_back({"БЖЧ", "ЛР", false, 4, 4, 5, 10, GradeBias::None, std::nullopt});

    r.push_back({"ООП", "ЛР", false, 8, 8, 4, 10, GradeBias::None, std::nullopt});

    r.push_back({"ОИнфБ", "ПЗ", false, 6, 8, 8, 10, GradeBias::None, std::nullopt});

    r.push_back({"БД", "ЛР", false, 6, 6, 9, 9, GradeBias::Fixed, 9});
    r.push_back({"БД", "ПЗ", false, 4, 4, 8, 10, GradeBias::None, std::nullopt});

    r.push_back({"МСиСвИТ", "ПЗ", false, 1, 10, 0, 10, GradeBias::OftenGE8, std::nullopt});

    return r;
}

int D1Randomizer::pickCount(const GradeRule& rule)
{
    if (rule.minCount <= 0) return 0;
    if (rule.maxCount < rule.minCount) return rule.minCount;

    std::uniform_int_distribution<int> dist(rule.minCount, rule.maxCount);
    return dist(rng);
}

int D1Randomizer::clampGrade(int v)
{
    if (v < 0) return 0;
    if (v > 10) return 10;
    return v;
}

int D1Randomizer::generateGradeValue(const GradeRule& rule)
{
    if (rule.fixedGrade.has_value()) {
        return clampGrade(*rule.fixedGrade);
    }

    const int lo = std::max(0, rule.minGrade);
    const int hi = std::min(10, rule.maxGrade);
    if (lo > hi) return lo;

    if (rule.bias == GradeBias::Often10) {
        if (hi >= 10 && lo <= 10) {
            std::uniform_int_distribution<int> p(1, 100);
            if (p(rng) <= 70) return 10;
        }
        std::uniform_int_distribution<int> dist(lo, hi);
        return dist(rng);
    }

    if (rule.bias == GradeBias::Often9_10) {
        std::vector<int> values;
        std::vector<int> weights;
        for (int v = lo; v <= hi; ++v) {
            values.push_back(v);
            int w = 10;
            if (v == 9 || v == 10) w = 35;
            weights.push_back(w);
        }
        std::discrete_distribution<int> dist(weights.begin(), weights.end());
        return values[dist(rng)];
    }

    if (rule.bias == GradeBias::OftenGE8) {
        std::vector<int> values;
        std::vector<int> weights;
        for (int v = lo; v <= hi; ++v) {
            values.push_back(v);
            int w = (v >= 8) ? 25 : 10;
            weights.push_back(w);
        }
        std::discrete_distribution<int> dist(weights.begin(), weights.end());
        return values[dist(rng)];
    }

    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
}

std::vector<std::string> D1Randomizer::pickRandomDistinct(
    std::mt19937& rng,
    const std::vector<std::string>& src,
    int count)
{
    if (count <= 0 || src.empty()) return {};

    std::vector<std::string> pool = src;
    pool = uniqueSorted(std::move(pool));

    if (count >= static_cast<int>(pool.size())) {
        return pool;
    }

    std::shuffle(pool.begin(), pool.end(), rng);
    pool.resize(count);
    pool = uniqueSorted(std::move(pool));
    return pool;
}

Result<std::vector<std::string>> D1Randomizer::collectLessonDatesForStudent(
    int groupId,
    int studentSubgroup,
    int subjectId,
    const std::string& lessonType,
    int semesterId)
{
    if (groupId <= 0) {
        return Result<std::vector<std::string>>::Fail("groupId must be > 0");
    }
    if (studentSubgroup < 0 || studentSubgroup > 2) {
        return Result<std::vector<std::string>>::Fail("studentSubgroup must be in [0..2]");
    }
    if (subjectId <= 0) {
        return Result<std::vector<std::string>>::Fail("subjectId must be > 0");
    }
    if (semesterId <= 0) {
        return Result<std::vector<std::string>>::Fail("semesterId must be > 0");
    }

    const std::string ltype = trimCopy(lessonType);
    if (ltype.empty()) {
        return Result<std::vector<std::string>>::Fail("lessonType is empty");
    }

    sqlite3* rawDb = db.rawHandle();
    if (!rawDb) {
        return Result<std::vector<std::string>>::Fail("DB not connected");
    }

    std::string semStart;
    std::string semEnd;
    if (!execGetSemesterDateRange(rawDb, semesterId, semStart, semEnd)) {
        return Result<std::vector<std::string>>::Fail("DB: failed to load semester date range");
    }

    const bool filterBySemesterDates = (!semStart.empty() && !semEnd.empty());

    std::vector<std::tuple<int, int, std::string, std::string>> weeks;
    if (!db.getCycleWeeks(weeks)) {
        return Result<std::vector<std::string>>::Fail("DB: getCycleWeeks failed");
    }

    std::vector<std::string> dates;

    for (const auto& w : weeks) {
        const int weekId = std::get<0>(w);
        const int weekOfCycle = std::get<1>(w);
        const std::string& weekStart = std::get<2>(w);
        const std::string& weekEnd = std::get<3>(w);

        if (filterBySemesterDates) {
            if (!weekEnd.empty() && weekEnd < semStart) continue;
            if (!weekStart.empty() && weekStart > semEnd) continue;
        }

        for (int weekday = 1; weekday <= 6; ++weekday) {
            std::vector<std::tuple<int, int, int, std::string, std::string, std::string, std::string>> sched;
            if (!db.getScheduleForGroup(groupId, weekday, weekOfCycle, sched)) {
                return Result<std::vector<std::string>>::Fail("DB: getScheduleForGroup failed");
            }

            bool hasTargetLesson = false;
            for (const auto& row : sched) {
                const int subgroup = std::get<2>(row);
                const std::string subjName = trimCopy(std::get<3>(row));
                const std::string type = trimCopy(std::get<5>(row));

                if (type != ltype) continue;

                int sid = 0;
                if (!db.getSubjectIdByName(subjName, sid)) {
                    return Result<std::vector<std::string>>::Fail("DB: getSubjectIdByName failed");
                }

                if (sid != subjectId) continue;

                if (!(subgroup == 0 || subgroup == studentSubgroup || studentSubgroup == 0)) {
                    continue;
                }

                hasTargetLesson = true;
                break;
            }

            if (!hasTargetLesson) continue;

            std::string dateISO;
            if (!db.getDateForWeekdayByWeekId(weekId, weekday, dateISO)) {
                return Result<std::vector<std::string>>::Fail("DB: getDateForWeekdayByWeekId failed");
            }
            if (dateISO.empty()) continue;

            if (filterBySemesterDates) {
                if (dateISO < semStart || dateISO > semEnd) continue;
            }

            dates.push_back(std::move(dateISO));
        }
    }

    dates = uniqueSorted(std::move(dates));
    return Result<std::vector<std::string>>::Ok(std::move(dates));
}

Result<D1RandomizerStats> D1Randomizer::generateForGroup(int groupId, int semesterId, bool overwriteExisting)
{
    if (groupId <= 0) {
        return Result<D1RandomizerStats>::Fail("groupId must be > 0");
    }
    if (semesterId <= 0) {
        return Result<D1RandomizerStats>::Fail("semesterId must be > 0");
    }

    sqlite3* rawDb = db.rawHandle();
    if (!rawDb) {
        return Result<D1RandomizerStats>::Fail("DB not connected");
    }

    std::vector<StudentRow> students;
    if (!execGetStudentsOfGroupWithSubgroup(rawDb, groupId, students)) {
        return Result<D1RandomizerStats>::Fail("DB: get students of group failed");
    }

    auto allRules = rules();

    struct ResolvedRule {
        GradeRule rule;
        int subjectId = 0;
    };

    std::vector<ResolvedRule> resolved;
    resolved.reserve(allRules.size());

    for (auto& rule : allRules) {
        rule.subjectName = trimCopy(rule.subjectName);
        rule.lessonType = trimCopy(rule.lessonType);

        if (rule.subjectName.empty() || rule.lessonType.empty()) {
            continue;
        }

        int subjectId = 0;
        if (!db.getSubjectIdByName(rule.subjectName, subjectId) || subjectId <= 0) {
            continue;
        }

        if (rule.minCount < 0) rule.minCount = 0;
        if (rule.maxCount < 0) rule.maxCount = 0;
        if (rule.maxCount < rule.minCount) rule.maxCount = rule.minCount;

        rule.minGrade = clampGrade(rule.minGrade);
        rule.maxGrade = clampGrade(rule.maxGrade);
        if (rule.maxGrade < rule.minGrade) rule.maxGrade = rule.minGrade;

        if (rule.fixedGrade.has_value()) {
            rule.fixedGrade = clampGrade(*rule.fixedGrade);
        }

        resolved.push_back({rule, subjectId});
    }

    D1RandomizerStats stats;

    if (!execBegin(rawDb)) {
        return Result<D1RandomizerStats>::Fail("DB: begin transaction failed");
    }

    bool ok = true;
    auto rollbackGuard = [&]() {
        if (!ok) execRollback(rawDb);
    };

    for (const auto& student : students) {
        if (student.id <= 0) continue;

        for (const auto& rr : resolved) {
            const auto& rule = rr.rule;

            const std::string ruleKey = rule.subjectName + " " + rule.lessonType;
            auto& report = stats.reportByRule[ruleKey];

            const int targetCount = pickCount(rule);
            if (targetCount <= 0) continue;

            report.requestedGrades += targetCount;

            if (rule.lessonType == "ЛК" && !rule.allowLecture) {
                stats.skippedBecauseLectureForbidden++;
                report.skippedNoCandidates += 1;
                std::cerr << "[D1] SKIP: lecture forbidden for rule '" << ruleKey
                          << "' studentId=" << student.id << "\n";
                continue;
            }

            std::vector<std::tuple<int, std::string, std::string>> existingGrades;
            if (!db.getStudentSubjectGrades(student.id, rr.subjectId, semesterId, existingGrades)) {
                ok = false;
                rollbackGuard();
                return Result<D1RandomizerStats>::Fail("DB: getStudentSubjectGrades failed");
            }

            std::vector<std::string> existingDates;
            existingDates.reserve(existingGrades.size());
            for (const auto& g : existingGrades) {
                existingDates.push_back(std::get<1>(g));
            }
            existingDates = uniqueSortedFilteredNonEmpty(std::move(existingDates));

            const int existingCount = static_cast<int>(existingDates.size());
            int remainingToCreate = targetCount - existingCount;
            if (remainingToCreate < 0) remainingToCreate = 0;

            std::vector<LessonOccurrence> occ;
            if (!db.getLessonOccurrencesForStudent(student.id, rr.subjectId, rule.lessonType, semesterId, occ)) {
                ok = false;
                rollbackGuard();
                return Result<D1RandomizerStats>::Fail("DB: getLessonOccurrencesForStudent failed");
            }

            std::vector<std::string> candidateDates;
            candidateDates.reserve(occ.size());
            for (const auto& o : occ) {
                candidateDates.push_back(o.lessonDateISO);
            }
            candidateDates = uniqueSortedFilteredNonEmpty(std::move(candidateDates));

            report.candidatesFound += static_cast<int>(candidateDates.size());
            if (candidateDates.empty()) {
                report.skippedNoCandidates += 1;
                std::cerr << "[D1] SKIP: no real lesson dates for rule '" << ruleKey
                          << "' studentId=" << student.id << "\n";
                continue;
            }

            if (!overwriteExisting) {
                std::unordered_set<std::string> existingSet(existingDates.begin(), existingDates.end());
                candidateDates.erase(
                    std::remove_if(candidateDates.begin(), candidateDates.end(), [&](const std::string& d) {
                        return existingSet.find(d) != existingSet.end();
                    }),
                    candidateDates.end()
                );
            }

            int toPick = overwriteExisting ? targetCount : remainingToCreate;
            if (toPick <= 0) {
                continue;
            }

            const auto pickedDates = pickRandomDistinct(rng, candidateDates, toPick);
            if (pickedDates.empty()) continue;

#ifndef NDEBUG
            std::unordered_set<std::string> candidatesSet(candidateDates.begin(), candidateDates.end());
#endif

            for (const auto& dateISO : pickedDates) {
                if (dateISO.empty()) continue;

#ifndef NDEBUG
                if (candidatesSet.find(dateISO) == candidatesSet.end()) {
                    ok = false;
                    rollbackGuard();
                    return Result<D1RandomizerStats>::Fail("Internal: picked date not in candidates");
                }
#endif

                const int value = generateGradeValue(rule);
                if (value < 0 || value > 10) {
                    ok = false;
                    rollbackGuard();
                    return Result<D1RandomizerStats>::Fail("Internal: generated grade out of [0..10]");
                }

                int existingId = 0;
                if (!db.findGradeId(student.id, rr.subjectId, semesterId, dateISO, existingId)) {
                    ok = false;
                    rollbackGuard();
                    return Result<D1RandomizerStats>::Fail("DB: findGradeId failed");
                }

                if (existingId > 0 && !overwriteExisting) {
                    stats.skippedExistingTotal++;
                    report.skippedExisting += 1;
                    continue;
                }

                if (!overwriteExisting && remainingToCreate <= 0) {
                    continue;
                }

                if (overwriteExisting && existingId <= 0 && existingCount >= targetCount) {
                    continue;
                }

                const std::string gradeType = "created by randomizer";
                const bool writeOk = db.upsertGradeByKey(student.id, rr.subjectId, semesterId, value, dateISO, gradeType);
                if (!writeOk) {
                    ok = false;
                    rollbackGuard();
                    return Result<D1RandomizerStats>::Fail("DB: upsertGradeByKey failed");
                }

                if (existingId > 0) {
                    stats.updatedExistingTotal++;
                    report.updated += 1;
                } else {
                    stats.createdTotal++;
                    report.created += 1;
                    stats.createdBySubject[rule.subjectName] += 1;
                    if (!overwriteExisting) {
                        remainingToCreate -= 1;
                    }
                }
            }
        }
    }

    std::cerr << "[D1] Report: skippedBecauseLectureForbidden=" << stats.skippedBecauseLectureForbidden << "\n";
    for (const auto& it : stats.reportByRule) {
        const auto& key = it.first;
        const auto& rep = it.second;
        std::cerr << "[D1] Rule '" << key << "': candidatesFound=" << rep.candidatesFound
                  << " requestedGrades=" << rep.requestedGrades
                  << " created=" << rep.created
                  << " updated=" << rep.updated
                  << " skippedExisting=" << rep.skippedExisting
                  << " skippedNoCandidates=" << rep.skippedNoCandidates
                  << "\n";
    }

    ok = execCommit(rawDb);
    if (!ok) {
        rollbackGuard();
        return Result<D1RandomizerStats>::Fail("DB: commit transaction failed");
    }

    return Result<D1RandomizerStats>::Ok(std::move(stats));
}
