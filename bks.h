//
// Created by Petr Flajsingr on 2019-03-23.
//

#ifndef PRL_1_BKS_H
#define PRL_1_BKS_H

enum class ProcType {
  Root, Node, List
};
std::string ProcTypeToString(ProcType procType);
class ProcInfo {
 public:
  ProcInfo(int id, int totalProc);
  /**
   * @return unique process id
   */
  int getId() const;
  /**
   * @return id of parent process
   */
  int getParentNodeId() const;
  /**
   * @return type of tree node
   */
  ProcType getType() const;
  friend std::ostream &operator<<(std::ostream &os, const ProcInfo &info);

 private:
  int id;
  int parentNodeId;
  ProcType type;
  /**
   * Calculate interval for node indices.
   */
  std::pair<int, int> getNodesInterval(int totalProc) const;

  const int rootId = 0;
};

#endif //PRL_1_BKS_H
