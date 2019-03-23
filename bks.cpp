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
  std::cout << procInfo << std::endl;
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
ProcInfo::ProcInfo(int id, int totalProc) : id(id), totalProc(totalProc) {
  parentNodeId = (id - 1) / 2;
  if (id == rootId) {
    type = ProcType::Root;
    parentNodeId = -1;
    procWorker = std::make_unique<RootMerger>(this);
    childrenIds = {1, 2};
  } else if (auto[sIndex, eIndex] = getNodesInterval(totalProc); sIndex <= id && id <= eIndex) {
    type = ProcType::Node;
    procWorker = std::make_unique<Merger>(this);
    childrenIds = {id * 2 + 1, id * 2 + 2};
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
int ProcInfo::getTreeLevel(int index) {
  if (index == 0) {
    return 0;
  } else {
    return getTreeLevel((index - 1) / 2) + 1;
  }
}
int ProcInfo::getInputSize() const {
  return (totalProc + 1) * 2 / static_cast<int>(pow(2, getTreeLevel(id)));
}
std::ostream &operator<<(std::ostream &os, const ProcInfo &info) {
  os << "ProcInfo:: id: " << info.id << ", parentNodeId: " << info.parentNodeId << ", type: "
     << ProcTypeToString(info.type);
  return os;
}
std::pair<int, int> ProcInfo::getNodesInterval(int totalProc) const {
  return std::make_pair(1, totalProc / 2 - 1);
}
const std::vector<int> &ProcInfo::getChildrenIds() const {
  return childrenIds;
}
int ProcInfo::getTotalProc() {
  return totalProc;
}
//\ ********** ProcInfo **********
ProcWorker::ProcWorker(ProcInfo *procInfo) : procInfo(procInfo) {}
void ProcWorker::sendToParent() {
  MPI_Send(data.data(), data.size(), MPI_INT, procInfo->getParentNodeId(), 0, MPI_COMM_WORLD);
  Logger::log("ProcWorker sent to parent " + std::to_string(procInfo->getParentNodeId())
                  + ", id " + std::to_string(procInfo->getId()));
}
void ProcWorker::receiveFromParent() {
  auto inSize = procInfo->getInputSize();
  int buffer[procInfo->getInputSize()];
  MPI_Recv(buffer, inSize, MPI_INT, procInfo->getParentNodeId(), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  for (int i = 0; i < inSize; ++i) {
    data.push_back(buffer[i]);
  }
  Logger::log("ProcWorker received from parent, id " + std::to_string(procInfo->getId()));
}
// ********** Sorter **********
void Sorter::run() {
  receiveAndSort();
  ProcWorker::sendToParent();
}
void Sorter::receiveAndSort() {
  ProcWorker::receiveFromParent();
  Logger::log("Sorter received, id " + std::to_string(procInfo->getId()));
  std::sort(data.begin(), data.end());
}
Sorter::Sorter(ProcInfo *procInfo) : ProcWorker(procInfo) {}
//\ ********** Sorter **********
// ********** Merger **********
Merger::Merger(ProcInfo *procInfo) : ProcWorker(procInfo) {}
void Merger::run() {
  ProcWorker::receiveFromParent();
  sendToChildren();
  data.clear();
  receiveAndMerge();
  ProcWorker::sendToParent();
}
void Merger::sendToChildren() {
  std::vector<int> data1(data.begin(), data.begin() + data.size() / 2);
  std::vector<int> data2(data.begin() + data.size() / 2, data.end());
  MPI_Send(data1.data(),
           static_cast<int>(data1.size()),
           MPI_INT,
           procInfo->getChildrenIds()[0],
           0,
           MPI_COMM_WORLD);
  MPI_Send(data2.data(),
           static_cast<int>(data2.size()),
           MPI_INT,
           procInfo->getChildrenIds()[1],
           0,
           MPI_COMM_WORLD);
  Logger::log("Merger sent to children " + std::to_string(procInfo->getChildrenIds()[0]) + " "
                  + std::to_string(procInfo->getChildrenIds()[1]) + ", id "
                  + std::to_string(procInfo->getId()));
}
void Merger::receiveAndMerge() {
  auto inSize = procInfo->getInputSize();
  int buffer[inSize];
  MPI_Recv(buffer, inSize, MPI_INT, procInfo->getChildrenIds()[0], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  std::vector<int> data1;
  data1.reserve(inSize);
  for (int i = 0; i < inSize / 2; ++i) {
    data1.push_back(buffer[i]);
  }
  MPI_Recv(buffer, inSize, MPI_INT, procInfo->getChildrenIds()[1], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  std::vector<int> data2;
  data2.reserve(inSize);
  for (int i = 0; i < inSize / 2; ++i) {
    data2.push_back(buffer[i]);
  }
  std::merge(data1.begin(), data1.end(),
             data2.begin(), data2.end(),
             std::back_inserter(data));
}
//\ ********** Merger **********
// ********** RootMerger **********
RootMerger::RootMerger(ProcInfo *procInfo) : Merger(procInfo) {}
void RootMerger::run() {
  readFile();
  if (procInfo->getTotalProc() > 1) {
    Merger::sendToChildren();
    data.clear();
    Merger::receiveAndMerge();
  } else {
    std::sort(data.begin(), data.end());
  }
  print();
}
void RootMerger::readFile() {
  std::ifstream ifstream("numbers", std::ios::binary);
  char val;
  while (ifstream.get(val)) {
    data.push_back(static_cast<uint8_t>(val));
  }
  for (int i = data.size(); i < procInfo->getInputSize(); ++i) {
    data.push_back(-1);
  }
}
void RootMerger::print() {
  for (const auto &val : data) {
    if (val != -1) {
      std::cout << val << std::endl;
    }
  }
}
//\ ********** RootMerger **********
