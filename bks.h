//
// Created by Petr Flajsingr on 2019-03-23.
//

#ifndef PRL_1_BKS_H
#define PRL_1_BKS_H

#include <utility>
#include <vector>
#include <memory>
#include <cmath>

class ProcInfo;
class ProcWorker {
 public:
  explicit ProcWorker(ProcInfo *procInfo);
  virtual void run() = 0;
  virtual ~ProcWorker() = default;

 protected:
  ProcInfo *procInfo;
  std::vector<int> data;

  void sendToParent();
  void receiveFromParent();
};
class Sorter : public ProcWorker {
 public:
  explicit Sorter(ProcInfo *procInfo);
  void run() override;

 protected:
  void receiveAndSort();
};
class Merger : public ProcWorker {
 public:
  explicit Merger(ProcInfo *procInfo);
  void run() override;

 protected:
  void sendToChildren();
  void receiveAndMerge();
};
class RootMerger : public Merger {
 public:
  explicit RootMerger(ProcInfo *procInfo);
  void run() override;

 private:
  void readFile();
  void print();
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
  const std::vector<int> &getChildrenIds() const;
  int getInputSize() const;
  /**
   * @return type of tree node
   */
  ProcType getType() const;
  friend std::ostream &operator<<(std::ostream &os, const ProcInfo &info);

  std::unique_ptr<ProcWorker> procWorker;

  static int getTreeLevel(int index);

 private:
  int id;
  int parentNodeId;
  ProcType type;
  std::vector<int> childrenIds;
  int totalProc;

  /**
   * Calculate interval for node indices.
   */
  std::pair<int, int> getNodesInterval(int totalProc) const;

  const int rootId = 0;
};

struct Logger {
  static bool allowLog;

  static void log(const char* msg) {
    if (!allowLog) {
      return;
    }
    std::cout << msg << std::endl;
  }

  static void log(const std::string &msg) {
    log(msg.c_str());
  }
};

#endif //PRL_1_BKS_H