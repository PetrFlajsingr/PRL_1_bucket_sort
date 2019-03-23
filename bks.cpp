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
  } else if (auto[sIndex, eIndex] = getNodesInterval(totalProc); sIndex <= id && id <= eIndex) {
    type = ProcType::Node;
  } else {
    type = ProcType::List;
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
