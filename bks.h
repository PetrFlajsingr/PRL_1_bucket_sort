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
  /**
   * Send data to parent (procInfo->getParentId())
   */
  void sendToParent();
  /**
   * Receive data from parent (procInfo->getParentId())
   */
  void receiveFromParent();
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

 protected:
  /**
   * Receive data from parent and sort them.
   */
  void receiveAndSort();
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
   * Send half of this->data to each child.
   */
  void sendToChildren();
  /**
   * Receive data from both children and merge them to this->data.
   */
  void receiveAndMerge();
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
  /*
   * Print this->data to stdout.
   */
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
  /**
   * Calculate node's level in tree by its index.
   * @param index index in tree
   * @return level in tree
   */
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

  static void log(const char *msg) {
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