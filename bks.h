//
// Created by Petr Flajsingr on 2019-03-23.
//

#ifndef PRL_1_BKS_H
#define PRL_1_BKS_H

#include <utility>
#include <vector>
#include <memory>
#include <cmath>
#include <string>


class ProcInfo;
/**
 * Base class for process execution.
 */
class ProcWorker {
 public:
  explicit ProcWorker(ProcInfo *procInfo);
  virtual void run() = 0;
  virtual ~ProcWorker() = default;

 protected:
  ProcInfo *procInfo;
  std::vector<int> data;
  std::vector<std::vector<int>> dataFromChildren;
  /**
   * Send data to parent (procInfo->getParentId())
   */
  void sendToParent();
  /**
   * Send data to children, divide them into buckets of equal size for each child.
   */
  void sendToChildren();
  /**
   * Receive data from parent (procInfo->getParentId()), save to this->data.
   */
  void receiveFromParent();
  /**
   * Receive data from children, save the in this->dataFromChildren.
   */
  void receiveFromChildren();
};
/**
 * Worker used in list processes to sort buckets.
 */
class Sorter : public ProcWorker {
 public:
  explicit Sorter(ProcInfo *procInfo);
  /**
   * Receive data from parent, sort them using std::sort() and send them back.
   */
  void run() override;
};
class Merger : public ProcWorker {
 public:
  explicit Merger(ProcInfo *procInfo);
  /**
   * Receive data from parent, send them to children, receive from children,
   * merge using std::merge() and send to parent.
   */
  void run() override;

 protected:
  /**
   * Merge data from children to this->data.
   */
  void merge();
};
/**
 * Worker for tree root.
 */
class RootMerger : public Merger {
 public:
  explicit RootMerger(ProcInfo *procInfo);
  /**
   * Read input data from file, add padding if necessary. Send them to children,
   * receive from children and print to stdout.
   */
  void run() override;

 private:
  /**
   * Read binary unsigned 1B data from file "./numbers" and store them in this->data.
   */
  void readFile();
  /**
   * Print this->data to stdout.
   */
  void print(bool printRow = false);
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
  /**
   * Calculate node's level in tree by its index.
   * @param index index in tree
   * @return level in tree
   */
  static int getTreeLevel(int index);
  /**
   * @return total amount of processes
   */
  int getTotalProc() const;
  std::string toString() const;

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
/**
 * Debug logger.
 */
struct Logger {
  static bool allowLog;
  static void log(const char *msg) {
    if (!allowLog) {
      return;
    }
    std::cout << msg << "\n";
  }
  static void log(const std::string &msg) {
    log(msg.c_str());
  }
};

#endif //PRL_1_BKS_H