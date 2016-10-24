#pragma once

#include <vector>

#include "common/god.h"
#include "common/history.h"
#include "common/utils.h"
#include "common/vocab.h"
#include "common/soft_alignment.h"
#include "common/utils.h"

template <class OStream>
void Printer(const History& history, size_t lineNo, OStream& out) {
  if(God::Get<bool>("n-best")) {
    std::vector<std::string> scorerNames = God::GetScorerNames();
    const NBestList &nbl = history.NBest(God::Get<size_t>("beam-size"));
    if(God::Get<bool>("wipo")) {
      out << "OUT: " << nbl.size() << std::endl;
    }
    for(size_t i = 0; i < nbl.size(); ++i) {
      const Result& result = nbl[i];
      const Words &words = result.first;
      const HypothesisPtr &hypo = result.second;

      if(God::Get<bool>("wipo"))
        out << "OUT: ";
      out << lineNo << " ||| " << Join(God::Postprocess(God::GetTargetVocab()(words))) << " |||";
      for(size_t j = 0; j < hypo->GetCostBreakdown().size(); ++j) {
        out << " " << scorerNames[j] << "= " << hypo->GetCostBreakdown()[j];
      }
      if(God::Get<bool>("normalize")) {
        out << " ||| " << hypo->GetCost() / words.size() << std::endl;
      }
      else {
        out << " ||| " << hypo->GetCost() << std::endl;
      }
    }
  }
  else
  {
    auto targetWordList = God::GetTargetVocab()(history.Top().first);
    std::string translation = Join(targetWordList);
    std::string postprocessedTranslation = Join(God::Postprocess(targetWordList));
    auto sourceWordList = history.sourceWordList;
    std::string source = Join(sourceWordList);
    LOG(progress) << "Source: " << source;
    LOG(progress) << "Translation: " << translation;

    out << postprocessedTranslation;
    const std::string seperator = "@@";

    if (God::Get<bool>("return-alignment")) {
      std::stringstream ss;
      auto last = history.Top().second;
      std::vector<SoftAlignment> aligns;
      while (last->GetPrevHyp().get() != nullptr) {
        aligns.push_back(*(last->GetAlignment(0)));
        last = last->GetPrevHyp();
      }

      if (God::Has("bpe")) {
        std::vector<int> splitSourceWords;
        for(int counter=0; counter< sourceWordList.size(); counter++) {
          if(EndsWith(sourceWordList[counter], seperator)){
            splitSourceWords.push_back(counter);
          }
        }
        std::vector<int> splitTargetWords;
        for(int counter=0; counter< targetWordList.size(); counter++) {
          if(EndsWith(targetWordList[counter], seperator)){
            splitTargetWords.push_back(counter);
          }
        }
        int targetWordCount = targetWordList.size() + 1 - splitTargetWords.size();
        int sourceWordCount = sourceWordList.size() + 1 - splitSourceWords.size();
	float compressedAlignment[targetWordCount][sourceWordCount];
	memset(compressedAlignment, 0,  sizeof compressedAlignment);

        int targetCounter = 0;
        bool lastTargetJoinable = false;
        for (int x = aligns.size() -1; x>=0; x--) {
          int sourceCounter = 0;
          for (int y = 0; y< aligns[x].size(); y++) {
            compressedAlignment[targetCounter][sourceCounter] += aligns[x][y];
	    if(std::find(splitSourceWords.begin(), splitSourceWords.end(), y) == splitSourceWords.end()){
              if(lastTargetJoinable){
                compressedAlignment[targetCounter][sourceCounter] /= 2;
              }
	      sourceCounter++;
            }
          }
          if(std::find(splitTargetWords.begin(), splitTargetWords.end(), aligns.size() - x - 1) == splitTargetWords.end()){
            targetCounter++;
            lastTargetJoinable = false;
          } else {
            lastTargetJoinable = true;
          }
        }

        for(int x=0;x < targetWordCount; x++){
            ss << "(";
          for(int y=0;y<sourceWordCount; y++){
            ss << compressedAlignment[x][y] << " ";
          }
            ss << ") | ";
        }

      } else {
    
        for (auto it = aligns.rbegin(); it != aligns.rend(); ++it) {
          ss << "(";
          for (auto sIt = it->begin(); sIt != it->end(); ++sIt) {
            ss << *sIt << " ";
          }
          ss << ") | ";
        }
      }
      out << " ||| " << ss.str();
    }
    out << std::endl;
  }
}
