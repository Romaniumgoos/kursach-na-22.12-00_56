#pragma once

struct ScholarshipInfo {
    double average;      // средний балл
    double coefficient;  // 1.0 / 1.2 / 1.4 / 1.6
    double amountBYN;    // сумма стипендии
};

class ScholarshipCalculator {
public:
    static ScholarshipInfo calculate(double avg);
};
