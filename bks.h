//
// Created by Petr Flajsingr on 2019-03-23.
//

#ifndef PRL_1_BKS_H
#define PRL_1_BKS_H

#include <utility>
#include <vector>
#include <memory>

class ProcWorker {
 public:
  virtual void run() = 0;
  virtual ~ProcWorker() = default;
};
class Sorter : public ProcWorker {
 public:
  void run() override;

 private:
  std::vector<int> data;
};
class Merger : public ProcWorker {
 public:
  void run() override;

 private:
  std::vector<int> data;
};


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

  std::unique_ptr<ProcWorker> procWorker;

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