#include <iostream>
#include <fstream>
#include <vector>
#include <mpi.h>
#include "bks.h"


int main(int, char **) {
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
ProcInfo::ProcInfo(int id, int totalProc) : id(id) {
  parentNodeId = id / 2;
  if (id == rootId) {
    type = ProcType::Root;
    parentNodeId = -1;
    procWorker = std::make_unique<Merger>();
  } else if (auto[sIndex, eIndex] = getNodesInterval(totalProc); sIndex <= id && id <= eIndex) {
    type = ProcType::Node;
    procWorker = std::make_unique<Merger>();
  } else {
    type = ProcType::List;
    procWorker = std::make_unique<Sorter>();
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
void Sorter::run() {
  std::cout << "Sorter running..." << std::endl;
}

void Merger::run() {
  std::cout << "Merger running..." << std::endl;
}
