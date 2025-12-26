#pragma once

#include "core/result.h"
#include "database.h"

#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

enum class GradeBias {
    None,
    Often10,
    Often9_10,
    OftenGE8,
    Fixed
};

struct GradeRule {
    std::string subjectName;
    std::string lessonType;

    bool allowLecture = false;

    int minCount = 0;
    int maxCount = 0;

    int minGrade = 0;
    int maxGrade = 10;

    GradeBias bias = GradeBias::None;
    std::optional<int> fixedGrade;
};

struct D1RandomizerStats {
    std::map<std::string, int> createdBySubject;
    int createdTotal = 0;
    int skippedExistingTotal = 0;
    int updatedExistingTotal = 0;

    struct RuleReport {
        int candidatesFound = 0;
        int requestedGrades = 0;
        int created = 0;
        int updated = 0;
        int skippedExisting = 0;
        int skippedNoCandidates = 0;
    };

    std::map<std::string, RuleReport> reportByRule;
    int skippedBecauseLectureForbidden = 0;
};

class D1Randomizer {
public:
    explicit D1Randomizer(Database& db);

    [[nodiscard]] Result<D1RandomizerStats> generateForGroup(int groupId, int semesterId, bool overwriteExisting);

private:
    Database& db;
    std::mt19937 rng;

    [[nodiscard]] std::vector<GradeRule> rules() const;

    [[nodiscard]] int pickCount(const GradeRule& rule);
    [[nodiscard]] int generateGradeValue(const GradeRule& rule);

    [[nodiscard]] static int clampGrade(int v);

    [[nodiscard]] Result<std::vector<std::string>> collectLessonDatesForStudent(
        int groupId,
        int studentSubgroup,
        int subjectId,
        const std::string& lessonType,
        int semesterId);

    [[nodiscard]] static std::vector<std::string> pickRandomDistinct(
        std::mt19937& rng,
        const std::vector<std::string>& src,
        int count);
};
