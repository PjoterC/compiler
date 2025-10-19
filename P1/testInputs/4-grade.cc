#include <vector>
#include "grade.h"
#include "median.h"
#include "Student_info.h"

using std::vector;


// compute a student's overall grade from midterm and final exam grades and homework grade
double grade(double midterm, double final, double homework)
{
	return 0.2 * midterm + 0.4 * final + 0.4 * homework;
}

// compute a student's overall grade from midterm and final exam grades
// and vector of homework grades.
// this function does not copy its argument, because `median' does so for us.
double grade(double midterm, double final, const vector_double& hw)
{
	if (hw.size() == 0)
		return -1;
	return grade(midterm, final, median(hw));
}

double grade(const Student_info& s)
{
	return grade(s.midterm, s.final, s.homework);
}

std::vector<double> grades(const std::vector<Student_info>& studentInfos) {
  int idx = 0;
  std::vector<double> out;
  out.resize(studentInfos.size());
  while(studentInfos.cbegin() + idx != studentInfos.cend()) {
    const double grade = grade(studentInfos[idx]);
    out[idx] = grade;
    idx = idx + 1;
  }
  return out;
}
