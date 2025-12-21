#include "scholarship.h"

ScholarshipInfo ScholarshipCalculator::calculate(double avg)
{
    ScholarshipInfo info{};
    info.average = avg;

    const double base = 157.49;

    if (avg >= 9.0 && avg <= 10.0) {
        info.coefficient = 1.6;
    } else if (avg >= 8.0 && avg < 9.0) {
        info.coefficient = 1.4;
    } else if (avg >= 6.0 && avg < 8.0) {
        info.coefficient = 1.2;
    } else if (avg >= 5.0 && avg < 6.0) {
        info.coefficient = 1.0;
    } else {
        info.coefficient = 0.0;
    }

    info.amountBYN = base * info.coefficient;
    return info;
}
