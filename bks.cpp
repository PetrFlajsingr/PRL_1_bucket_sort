#include <iostream>
#include <fstream>
#include <vector>
#include <mpi.h>
#include "bks.h"
bool Logger::allowLog = false;

int main(int, char **) {
  Logger::allowLog = false;
  MPI_Init(nullptr, nullptr);

  int procCount;
  MPI_Comm_size(MPI_COMM_WORLD, &procCount);
  int procId;
  MPI_Comm_rank(MPI_COMM_WORLD, &procId);

  ProcInfo procInfo(procId, procCount);
  Logger::log(procInfo.toString());
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
  os << info.toString();
  return os;
}
std::pair<int, int> ProcInfo::getNodesInterval(int totalProc) const {
  return std::make_pair(1, totalProc / 2 - 1);
}
const std::vector<int> &ProcInfo::getChildrenIds() const {
  return childrenIds;
}
int ProcInfo::getTotalProc() const {
  return totalProc;
}
std::string ProcInfo::toString() const {
  return "ProcInfo:: id: " + std::to_string(id) + ", parentNodeId: " + std::to_string(parentNodeId)
      + ", type: " + ProcTypeToString(type);
}
//\ ********** ProcInfo **********
ProcWorker::ProcWorker(ProcInfo *procInfo) : procInfo(procInfo) {}
void ProcWorker::sendToParent() {
  MPI_Send(data.data(), data.size(), MPI_INT, procInfo->getParentNodeId(), 0, MPI_COMM_WORLD);
  Logger::log("Sent to parent " + std::to_string(procInfo->getParentNodeId())
                  + ", id " + std::to_string(procInfo->getId()));
}
void ProcWorker::sendToChildren() {
  int step = data.size() / procInfo->getChildrenIds().size();
  int startIndex = 0;
  int endIndex = step;
  for (auto childId : procInfo->getChildrenIds()) {
    std::vector<int> toSend(data.begin() + startIndex, data.begin() + endIndex);
    MPI_Send(toSend.data(),
             static_cast<int>(toSend.size()),
             MPI_INT,
             childId,
             0,
             MPI_COMM_WORLD);
    startIndex = endIndex;
    endIndex += step;
  }
  Logger::log("Merger sent to children " + std::to_string(procInfo->getChildrenIds()[0]) + " "
                  + std::to_string(procInfo->getChildrenIds()[1]) + ", id "
                  + std::to_string(procInfo->getId()));
}
void ProcWorker::receiveFromParent() {
  auto inSize = procInfo->getInputSize();
  int buffer[procInfo->getInputSize()];
  MPI_Recv(buffer, inSize, MPI_INT, procInfo->getParentNodeId(), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  for (int i = 0; i < inSize; ++i) {
    data.push_back(buffer[i]);
  }
  Logger::log("Received from parent, id " + std::to_string(procInfo->getId()));
}
void ProcWorker::receiveFromChildren() {
  dataFromChildren.clear();
  auto inSize = procInfo->getInputSize();
  int buffer[procInfo->getInputSize()];
  for (auto childId : procInfo->getChildrenIds()) {
    MPI_Recv(buffer, inSize, MPI_INT, childId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    std::vector<int> receivedData;
    for (int j = 0; j < inSize / procInfo->getChildrenIds().size(); ++j) {
      receivedData.push_back(buffer[j]);
    }
    dataFromChildren.push_back(receivedData);
  }
  Logger::log("Received from children, id " + std::to_string(procInfo->getId()));
}
// ********** Sorter **********
void Sorter::run() {
  receiveFromParent();
  std::sort(data.begin(), data.end());
  sendToParent();
}
Sorter::Sorter(ProcInfo *procInfo) : ProcWorker(procInfo) {}
//\ ********** Sorter **********
// ********** Merger **********
Merger::Merger(ProcInfo *procInfo) : ProcWorker(procInfo) {}
void Merger::run() {
  receiveFromParent();
  sendToChildren();
  data.clear();
  receiveFromChildren();
  merge();
  sendToParent();
}
void Merger::merge() {
  std::merge(dataFromChildren[0].begin(), dataFromChildren[0].end(),
             dataFromChildren[1].begin(), dataFromChildren[1].end(),
             std::back_inserter(data));
}
//\ ********** Merger **********
// ********** RootMerger **********
RootMerger::RootMerger(ProcInfo *procInfo) : Merger(procInfo) {}
void RootMerger::run() {
  readFile();
  print(true);
  if (procInfo->getTotalProc() > 1) {
    sendToChildren();
    data.clear();
    receiveFromChildren();
    merge();
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
void RootMerger::print(bool printRow) {
  for (const auto &val : data) {
    if (val != -1) {
      std::cout << val;
      if (printRow) {
        std::cout << " ";
      } else {
        std::cout << std::endl;
      }
    }
  }
  if (printRow) {
    std::cout << std::endl;
  }
}
//\ ********** RootMerger **********
