#include <iostream>
#include <fstream>
#include <vector>
#include <mpi.h>

enum class ProcType {
  Root, Node, List
};
std::string ProcTypeToString(ProcType procType) {
  switch (procType) {
    case ProcType::Root: return "Root";
    case ProcType::Node: return "Node";
    case ProcType::List: return "List";
  }
}
class ProcInfo {
 public:
  ProcInfo(int id, int totalProc) : id(id) {
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
  /**
   * @return unique process id
   */
  int getId() const {
    return id;
  }
  /**
   * @return id of parent process
   */
  int getParentNodeId() const {
    return parentNodeId;
  }
  /**
   * @return type of tree node
   */
  ProcType getType() const {
    return type;
  }
  friend std::ostream &operator<<(std::ostream &os, const ProcInfo &info) {
    os << "ProcInfo:: id: " << info.id << ", parentNodeId: " << info.parentNodeId << ", type: "
       << ProcTypeToString(info.type);
    return os;
  }

 private:
  int id;
  int parentNodeId;
  ProcType type;
  /**
   * Calculate interval for node indices.
   */
  std::pair<int, int> getNodesInterval(int totalProc) const {
    return std::make_pair(1, totalProc / 2 - 1);
  }

  const int rootId = 0;
};

int main(int argc, char **argv) {
  MPI_Init(nullptr, nullptr);

  int procCount;
  MPI_Comm_size(MPI_COMM_WORLD, &procCount);

  int procId;
  MPI_Comm_rank(MPI_COMM_WORLD, &procId);

  ProcInfo procInfo(procId, procCount);

  std::cout << procInfo << std::endl;

  MPI_Finalize();
}
