//
// Created by Petr Flajsingr on 2019-03-23.
//

#include <mpi.h>
#include <fstream>
#include <vector>
#include "bks.h"

int main(int, char **) {
  MPI_Init(nullptr, nullptr);

  int procCount;
  MPI_Comm_size(MPI_COMM_WORLD, &procCount);
  int procId;
  MPI_Comm_rank(MPI_COMM_WORLD, &procId);

  ProcInfo procInfo(procId, procCount);
  procInfo.procWorker->run();

  MPI_Finalize();
}

// ********** ProcInfo **********
ProcInfo::ProcInfo(int id, int totalProc) : id(id), totalProc(totalProc) {
  parentNodeId = (id - 1) / 2;
  if (id == rootId) {
    parentNodeId = -1;
    procWorker = std::make_unique<RootMerger>(this);
    childrenIds = {1, 2};
  } else if (auto[sIndex, eIndex] = getNodesInterval(totalProc);
      sIndex <= id && id <= eIndex) {
    procWorker = std::make_unique<Merger>(this);
    childrenIds = {id * 2 + 1, id * 2 + 2};
  } else {
    procWorker = std::make_unique<Sorter>(this);
  }
}
int ProcInfo::getTreeLevel(int index) {
  if (index == 0) {
    return 0;
  } else {
    return getTreeLevel((index - 1) / 2) + 1;
  }
}
std::pair<int, int> ProcInfo::getNodesInterval(int totalProc) const {
  return std::make_pair(1, totalProc / 2 - 1);
}
std::vector<int> ProcInfo::getListIds() const {
  auto[sIndex, eIndex] = getNodesInterval(totalProc);
  std::vector<int> result;
  for (int i = eIndex + 1; i < totalProc; ++i) {
    result.push_back(i);
  }
  return result;
}
//\ ********** ProcInfo **********
ProcWorker::ProcWorker(ProcInfo *procInfo) : procInfo(procInfo) {}
void ProcWorker::sendToParent() {
  sendDataSize(procInfo->parentNodeId, data.size());
  MPI_Send(data.data(), static_cast<int>(data.size()), MPI_INT, procInfo->parentNodeId, 0, MPI_COMM_WORLD);
}
void ProcWorker::sendToChildren() {
  int step = static_cast<int>(data.size() / procInfo->childrenIds.size());
  int startIndex = 0;
  int endIndex = step;
  for (const auto childId : procInfo->childrenIds) {
    std::vector<int> toSend(data.begin() + startIndex, data.begin() + endIndex);
    sendDataSize(childId, toSend.size());
    MPI_Send(toSend.data(),
             static_cast<int>(toSend.size()),
             MPI_INT,
             childId,
             0,
             MPI_COMM_WORLD);
    startIndex = endIndex;
    endIndex += step;
  }
}
void ProcWorker::sendToLists() {
  auto listIds = procInfo->getListIds();
  int step = static_cast<int>(data.size() / listIds.size());
  int startIndex = 0;
  int endIndex = step;
  for (const auto listId : listIds) {
    std::vector<int> toSend(data.begin() + startIndex, data.begin() + endIndex);
    sendDataSize(listId, toSend.size());
    MPI_Send(toSend.data(),
             static_cast<int>(toSend.size()),
             MPI_INT,
             listId,
             0,
             MPI_COMM_WORLD);
    startIndex = endIndex;
    endIndex += step;
  }
}
void ProcWorker::receiveFromParent() {
  auto inSize = receiveDataSize(procInfo->parentNodeId);
  int buffer[inSize];
  MPI_Recv(buffer, inSize, MPI_INT, procInfo->parentNodeId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  for (int i = 0; i < inSize; ++i) {
    data.push_back(buffer[i]);
  }
}
void ProcWorker::receiveFromChildren() {
  dataFromChildren.clear();
  for (auto childId : procInfo->childrenIds) {
    auto inSize = receiveDataSize(childId);
    int buffer[inSize];
    MPI_Recv(buffer, inSize, MPI_INT, childId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    std::vector<int> receivedData;
    for (int j = 0; j < inSize; ++j) {
      receivedData.push_back(buffer[j]);
    }
    dataFromChildren.push_back(receivedData);
  }
}
void ProcWorker::receiveFromRoot() {
  auto inSize = receiveDataSize(procInfo->rootId);
  int buffer[inSize];
  MPI_Recv(buffer, inSize, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  for (int i = 0; i < inSize; ++i) {
    data.push_back(buffer[i]);
  }
}
void ProcWorker::sendDataSize(int destRank, int size) {
  MPI_Send(&size, 1, MPI_INT, destRank, 0, MPI_COMM_WORLD);
}
int ProcWorker::receiveDataSize(int srcRank) {
  int result;
  MPI_Recv(&result, 1, MPI_INT, srcRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  return result;
}
// ********** Sorter **********
void Sorter::run() {
  receiveFromRoot();
  std::sort(data.begin(), data.end());
  sendToParent();
}
Sorter::Sorter(ProcInfo *procInfo) : ProcWorker(procInfo) {}
//\ ********** Sorter **********
// ********** Merger **********
Merger::Merger(ProcInfo *procInfo) : ProcWorker(procInfo) {}
void Merger::run() {
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
  if (procInfo->totalProc > 1) {
    sendToLists();
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
  for (int i = 0; i < data.size() % procInfo->getListIds().size(); ++i) {
    data.push_back(-1);
  }
}
void RootMerger::print(bool printRow) {
  for (int i = 0; i < data.size() - 1; ++i) {
    if (data[i] == -1) {
      continue;
    }
    std::cout << data[i];
    if (printRow) {
      std::cout << " ";
    } else {
      std::cout << std::endl;
    }
  }
  if (data.back() != -1) {
    std::cout << data.back() << std::endl;
  } else {
    std::cout << std::endl;
  }
}
//\ ********** RootMerger **********
