#pragma once
#include "model_db.hxx"

namespace cses {

void addForJudging(SubmissionPtr submission);
void updateJudgeHosts();
void compileEvaluator(TaskPtr task);

}
