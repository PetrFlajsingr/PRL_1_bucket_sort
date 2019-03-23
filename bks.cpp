#include <iostream>
#include <fstream>
#include <vector>
#include <mpi.h>
#include "bks.h"
bool Logger::allowLog = false;

int main(int, char **) {
  Logger::allowLog = true;
  MPI_Init(nullptr, nullptr);

  int procCount;
  MPI_Comm_size(MPI_COMM_WORLD, &procCount);

  int procId;
  MPI_Comm_rank(MPI_COMM_WORLD, &procId);

  ProcInfo procInfo(procId, procCount);
  procInfo.procWorker->run();

  MPI_Finalize();
}

std::string ProcTypeToString(ProcType procType) {
  switch (procType) {
    case ProcType::Root: return "Root";
    case ProcType::Node: return "Node";
    case ProcType::List: return "List";
  }
}
// ********** ProcInfo **********
ProcInfo::ProcInfo(int id, int totalProc) : id(id) {
  parentNodeId = id / 2;
  if (id == rootId) {
    type = ProcType::Root;
    parentNodeId = -1;
    procWorker = std::make_unique<RootMerger>(this);
  } else if (auto[sIndex, eIndex] = getNodesInterval(totalProc); sIndex <= id && id <= eIndex) {
    type = ProcType::Node;
    procWorker = std::make_unique<Merger>(this);
  } else {
    type = ProcType::List;
    procWorker = std::make_unique<Sorter>(this);
  }
}
int ProcInfo::getId() const {
  return id;
}
int ProcInfo::getParentNodeId() const {
  return parentNodeId;
}
ProcType ProcInfo::getType() const {
  return type;
}
std::ostream &operator<<(std::ostream &os, const ProcInfo &info) {
  os << "ProcInfo:: id: " << info.id << ", parentNodeId: " << info.parentNodeId << ", type: "
     << ProcTypeToString(info.type);
  return os;
}
std::pair<int, int> ProcInfo::getNodesInterval(int totalProc) const {
  return std::make_pair(1, totalProc / 2 - 1);
}
//\ ********** ProcInfo **********
ProcWorker::ProcWorker(ProcInfo *procInfo) : procInfo(procInfo) {}
// ********** Sorter **********
void Sorter::run() {
  Logger::log("Sorter run start, id " + std::to_string(procInfo->getId()));
  receiveAndSort();
  sendToParent();
}
void Sorter::receiveAndSort() {
  // TODO: receive
  Logger::log("Sorter received, id " + std::to_string(procInfo->getId()));
  data = {};
  std::sort(data.begin(), data.end());
  Logger::log("Sorter sorted, id " + std::to_string(procInfo->getId()));
}
void Sorter::sendToParent() {
  // TODO: send data
  Logger::log("Sorter sent data, id " + std::to_string(procInfo->getId()));
}
Sorter::Sorter(ProcInfo *procInfo) : ProcWorker(procInfo) {}
//\ ********** Sorter **********
// ********** Merger **********
Merger::Merger(ProcInfo *procInfo) : ProcWorker(procInfo) {}
void Merger::run() {
  Logger::log("Merger run start, id " + std::to_string(procInfo->getId()));
  receiveAndMerge();
  sendToParent();
}
void Merger::receiveAndMerge() {
  // TODO: receive
  Logger::log("Merger received, id " + std::to_string(procInfo->getId()));
  auto data1 = std::vector<int>{};
  auto data2 = std::vector<int>{};
  std::merge(data1.begin(), data1.end(),
             data2.begin(), data2.end(),
             std::back_inserter(data));
  Logger::log("Merger merged, id " + std::to_string(procInfo->getId()));
}
void Merger::sendToParent() {
  // TODO: send data
  Logger::log("Merger sent data, id " + std::to_string(procInfo->getId()));
}
//\ ********** Merger **********
// ********** RootMerger **********
RootMerger::RootMerger(ProcInfo *procInfo) : Merger(procInfo) {}
void RootMerger::run() {
  Logger::log("RootMerger run start, id " + std::to_string(procInfo->getId()));
  receiveAndPrint();
}
void RootMerger::receiveAndPrint() {
  // TODO: receive
  Logger::log("RootMerger received, id " + std::to_string(procInfo->getId()));
  data = {};
  for (const auto &val : data) {
    std::cout << val << std::endl;
  }
}
//\ ********** RootMerger **********
