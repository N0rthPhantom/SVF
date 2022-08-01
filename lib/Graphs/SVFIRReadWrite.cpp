//===- SVFIRReadWrite.cpp -- SVFIR read & write-----------------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013-2022>  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

/*
 * SVFIRReadWrite.cpp
 *
 *  Created on: July 30, 2022
 *      Author: Yuhao Gao
 */

#include "MemoryModel/SVFIR.h"
#include "Util/cJSON.h"
#include <fstream>

using namespace SVF;
using namespace SVFUtil;

/*!
 * Write SVFIR into a file
 */
void SVFIR::writeToFile(std::string fileName){

    // the json object we will export in the end
    cJSON* SVFIRJsonObj = cJSON_CreateObject();
    cJSON* SVFIRJsonObjNodes = cJSON_AddArrayToObject(SVFIRJsonObj, "nodes");
    cJSON* SVFIRJsonObjEdges = cJSON_AddArrayToObject(SVFIRJsonObj, "edges");

    // traverse the nodes in PAG
    for (Map<NodeID, SVFVar*>::iterator nodeIter = pag->begin(), endOfNodeIter = pag->end(); nodeIter != endOfNodeIter; ++nodeIter){

        NodeID nodeID = (*nodeIter).first;
        SVFVar* node = (*nodeIter).second;

        // get the kind_ID of this node
        SVF::s64_t nodeKindID = node->getNodeKind();
        assert( SVFVar::PNODEK::ValNode <= nodeKindID && nodeKindID <= SVFVar::PNODEK::DummyObjNode && "new SVFIR node kind?");
        // then transfer the kind_ID into a string
        std::string nodeKindStr = "UnknownKindNode";
        Map<SVFVar::PNODEK, std::string> nodeKindToStrMap = {
            {SVFVar::PNODEK::ValNode, "ValNode"},
            {SVFVar::PNODEK::GepValNode, "GepValNode"},
            {SVFVar::PNODEK::RetNode, "RetNode"},
            {SVFVar::PNODEK::VarargNode, "VarargNode"},
            {SVFVar::PNODEK::DummyValNode, "DummyValNode"},
            {SVFVar::PNODEK::ObjNode, "ObjNode"},
            {SVFVar::PNODEK::GepObjNode, "GepObjNode"},
            {SVFVar::PNODEK::FIObjNode, "FIObjNode"},
            {SVFVar::PNODEK::DummyObjNode, "DummyObjNode"},
        };
        Map<SVFVar::PNODEK, std::string>::iterator nodeKindToStrMapIter = nodeKindToStrMap.find((SVFVar::PNODEK)nodeKindID);
        assert(nodeKindToStrMapIter!=nodeKindToStrMap.end() && "new SVFIR node kind?");
        nodeKindStr = (*nodeKindToStrMapIter).second;

        // add node to the JSON array
        cJSON* nodeJSONObject = cJSON_CreateObject();
        cJSON_AddNumberToObject(nodeJSONObject, "nodeID", nodeID);
        cJSON_AddNumberToObject(nodeJSONObject, "nodeKindID", nodeID);
        cJSON_AddStringToObject(nodeJSONObject, "nodeKindStr", nodeKindStr.c_str());
        cJSON_AddBoolToObject(nodeJSONObject, "isIsolated", node->isIsolatedNode());
        cJSON_AddItemToArray(SVFIRJsonObjNodes, nodeJSONObject);
        
    }
    // END: traverse the nodes in PAG

    // traverse the edges in PAG
    Map<SVFStmt::PEDGEK, std::string> edgeKindToStrMap = {
        {SVFStmt::Addr, "Addr"},
        {SVFStmt::Copy, "Copy"},
        {SVFStmt::Store, "Store"},
        {SVFStmt::Load, "Load"},
        {SVFStmt::Call, "Call"},
        {SVFStmt::Ret, "Ret"},
        {SVFStmt::Gep, "Gep"},
        {SVFStmt::Phi, "Phi"},
        {SVFStmt::Select, "Select"},
        {SVFStmt::Cmp, "Cmp"},
        {SVFStmt::BinaryOp, "BinaryOp"},
        {SVFStmt::UnaryOp, "UnaryOp"},
        {SVFStmt::Branch, "Branch"},
        {SVFStmt::ThreadFork, "ThreadFork"},
        {SVFStmt::ThreadJoin, "ThreadJoin"}};
    for (Map<SVFStmt::PEDGEK, std::string>::iterator edgeKindIter = edgeKindToStrMap.begin(), endOfEdgeKindIter = edgeKindToStrMap.end();
         edgeKindIter != endOfEdgeKindIter; ++edgeKindIter)
    {
        SVFStmt::PEDGEK edgeKindID = (*edgeKindIter).first;
        std::string edgeKindStr = (*edgeKindIter).second;

        SVFStmt::SVFStmtSetTy &edges = pag->getSVFStmtSet(edgeKindID);
        for (SVFStmt::SVFStmtSetTy::iterator edgeIter = edges.begin(), endOfEdgeIter = edges.end(); edgeIter != endOfEdgeIter; ++edgeIter)
        {
            NodeID srcNodeID = (*edgeIter)->getSrcID();
            NodeID dstNodeID = (*edgeIter)->getDstID();

            // add edge to the JSON array
            cJSON *edgeJSONObject = cJSON_CreateObject();
            cJSON_AddNumberToObject(edgeJSONObject, "edgeKindID", edgeKindID);
            cJSON_AddStringToObject(edgeJSONObject, "edgeKindStr", edgeKindStr.c_str());
            cJSON_AddNumberToObject(edgeJSONObject, "srcNodeID", srcNodeID);
            cJSON_AddNumberToObject(edgeJSONObject, "dstNodeID", dstNodeID);

            cJSON_AddItemToArray(SVFIRJsonObjEdges, edgeJSONObject);
        }
    }
    // END: traverse the edges in PAG

    // write the JSON object to a file
    outs() << "Writing SVFIR to file: '" << fileName << "'...";
    std::fstream f(fileName.c_str(), std::ios_base::out);
    if (!f.good())
    {
        outs() << "  error opening file for writing!\n";
        cJSON_Delete(SVFIRJsonObj);
        return;
    }
    else
    {
        char *SVFIRJsonStr = cJSON_Print(SVFIRJsonObj);
        f << SVFIRJsonStr;
        f.close();
        cJSON_Delete(SVFIRJsonObj);
        free(SVFIRJsonStr);
        if (f.good())
        {
            outs() << "\n";
            return;
        }
    }
}

/*!
 * Read SVFIR from a file
 */
bool SVFIR::readFromFile(std::string fileName){
    outs() << "Read SVFIR from file: '" << fileName << "'...";

    // read string from file
    std::fstream f(fileName.c_str(), std::ios_base::in);
    int jsonFileLength = 0;
    f.seekg(0, std::ios::end);
    jsonFileLength = f.tellg();
    f.seekg(0, std::ios::beg);
    char* jsonFileBuffer = new char[jsonFileLength];
    f.read(jsonFileBuffer, jsonFileLength);
    f.close();
    
    // parse string into json object
    const cJSON* SVFIRJsonObj = cJSON_ParseWithLength(jsonFileBuffer, jsonFileLength);
    assert(SVFIRJsonObj && "Parse SVFIR file failed.");

    // parse nodes array
    const cJSON* SVFIRJsonObjNodes = cJSON_GetObjectItem(SVFIRJsonObj, "nodes");
    assert(SVFIRJsonObjNodes && cJSON_IsArray(SVFIRJsonObjNodes) && "Parse SVFIR nodes failed.");

    // parse edges array
    const cJSON* SVFIRJsonObjEdges = cJSON_GetObjectItem(SVFIRJsonObj, "edges");
    assert(SVFIRJsonObjEdges && cJSON_IsArray(SVFIRJsonObjEdges) && "Parse SVFIR edges failed.");
    
    // traverse nodes
    const cJSON* SVFIRJsonObjNode = NULL;
    cJSON_ArrayForEach(SVFIRJsonObjNode, SVFIRJsonObjNodes){
        // get nodeID
        const cJSON* SVFIRJsonObjNodeID = cJSON_GetObjectItem(SVFIRJsonObjNode, "nodeID");
        assert(SVFIRJsonObjNodeID && cJSON_IsNumber(SVFIRJsonObjNodeID) && "Parse SVFIR nodeID failed.");
        int nodeID = SVFIRJsonObjNodeID->valueint;

        // get node kind ID
        const cJSON* SVFIRJsonObjNodeKindID = cJSON_GetObjectItem(SVFIRJsonObjNode, "nodeKindID");
        assert(SVFIRJsonObjNodeKindID && cJSON_IsNumber(SVFIRJsonObjNodeKindID) && "Parse SVFIR nodeKindID failed.");
        int nodeKindID = SVFIRJsonObjNodeKindID->valueint;
        
        // add node to pag, record new nodeID in a map
        Map<int, int> nodeIDtoNewIDMap;
        assert( SVFVar::PNODEK::ValNode <= nodeKindID && nodeKindID <= SVFVar::PNODEK::DummyObjNode && "new SVFIR node kind?");
        switch (nodeKindID)
        {
        case SVFVar::PNODEK::ValNode:
            pag->addValNode(nullptr, nodeID);
            break;
        case SVFVar::PNODEK::GepValNode:
            SVF::LocationSet* ls = new SVF::LocationSet();
            pag->addGepValNode(nullptr,nullptr, *ls, nodeID, nullptr);
            break;
        case SVFVar::PNODEK::RetNode:
            pag->addRetNode(nullptr, nodeID);
            break;
        case SVFVar::PNODEK::VarargNode:
            pag->addVarargNode(nullptr, nodeID);
            break;
        case SVFVar::PNODEK::DummyValNode:
            NodeID newNodeID = pag->addDummyValNode();
            nodeIDtoNewIDMap[newNodeID] = nodeID;
            break;
        case SVFVar::PNODEK::ObjNode:
            pag->addObjNode(nullptr, nodeID);
            break;
        case SVFVar::PNODEK::GepObjNode:
            pag->addGepObjNode(nullptr, nodeID);
            break;
        case SVFVar::PNODEK::FIObjNode:
            NodeID newNodeID = pag->addFIObjNode(nullptr);
            nodeIDtoNewIDMap[newNodeID] = nodeID;
            break;
        case SVFVar::PNODEK::DummyObjNode:
            NodeID newNodeID = pag->addDummyObjNode(nullptr);
            nodeIDtoNewIDMap[newNodeID] = nodeID;
            break;
        
        default:
            break;
        }

    }

    const cJSON* SVFIRJsonObjEdge = NULL;
    cJSON_ArrayForEach(SVFIRJsonObjEdge, SVFIRJsonObjEdges){
        // cJSON_AddNumberToObject(edgeJSONObject, "edgeKindID", edgeKindID);
        // cJSON_AddNumberToObject(edgeJSONObject, "srcNodeID", srcNodeID);
        // cJSON_AddNumberToObject(edgeJSONObject, "dstNodeID", dstNodeID);
        
        // get srcNodeID
        const cJSON* SVFIRJsonObjEdgeSrcNodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "srcNodeID");
        assert(SVFIRJsonObjEdgeSrcNodeID && cJSON_IsNumber(SVFIRJsonObjEdgeSrcNodeID) && "Parse SVFIR srcNodeID failed.");
        NodeID* srcNodeID = new NodeID(SVFIRJsonObjEdgeSrcNodeID->valueint);
        
        // get dstNodeID
        const cJSON* SVFIRJsonObjEdgeDstNodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "dstNodeID");
        assert(SVFIRJsonObjEdgeDstNodeID && cJSON_IsNumber(SVFIRJsonObjEdgeDstNodeID) && "Parse SVFIR dstNodeID failed.");
        NodeID* dstNodeID = new NodeID(SVFIRJsonObjEdgeDstNodeID->valueint);

        // get edge kind ID
        const cJSON* SVFIRJsonObjEdgeKindID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "edgeKindID");
        assert(SVFIRJsonObjEdgeKindID && cJSON_IsNumber(SVFIRJsonObjEdgeKindID) && "Parse SVFIR edgeKindID failed.");
        int edgeKindID = SVFIRJsonObjEdgeKindID->valueint;

        // {SVFStmt::Addr, "Addr"},
        // {SVFStmt::Copy, "Copy"},
        // {SVFStmt::Store, "Store"},
        // {SVFStmt::Load, "Load"},
        // {SVFStmt::Call, "Call"},
        // {SVFStmt::Ret, "Ret"},
        // {SVFStmt::Gep, "Gep"},
        // {SVFStmt::Phi, "Phi"},
        // {SVFStmt::Select, "Select"},
        // {SVFStmt::Cmp, "Cmp"},
        // {SVFStmt::BinaryOp, "BinaryOp"},
        // {SVFStmt::UnaryOp, "UnaryOp"},
        // {SVFStmt::Branch, "Branch"},
        // {SVFStmt::ThreadFork, "ThreadFork"},
        // {SVFStmt::ThreadJoin, "ThreadJoin"}};

        assert( SVFStmt::Addr <= edgeKindID && edgeKindID <= SVFStmt::ThreadJoin && "new SVFIR edge kind?");
        switch (edgeKindID)
        {
        case SVFStmt::Addr:
            pag->addAddrStmt(*srcNodeID, *dstNodeID);
            break;
        case SVFStmt::Copy:
            pag->addCopyStmt(*srcNodeID, *dstNodeID);
            break;
        case SVFStmt::Store:
            pag->addStoreStmt(*srcNodeID, *dstNodeID, nullptr);
            break;
        case SVFStmt::Load:
            pag->addLoadStmt(*srcNodeID, *dstNodeID);
            break;
        case SVFStmt::Call:
            pag->addCallPE(*srcNodeID, *dstNodeID, nullptr, nullptr);
            break;
        // case SVFStmt::Ret:
        //     pag->addCallPE(*srcNodeID, *dstNodeID, nullptr, nullptr);
        //     break;
        case SVFStmt::Gep:
            SVF::LocationSet* ls = new SVF::LocationSet();
            pag->addGepStmt(*srcNodeID, *dstNodeID, *ls, false);
            break;
        case SVFStmt::Phi:
            pag->addPhiStmt(*srcNodeID, *dstNodeID, nullptr);
            break;
        // case SVFStmt::Select:
        //     pag->addSelectStmt(*srcNodeID, *dstNodeID);
        //     break;
        // case SVFStmt::Cmp:
        //     pag->addCmpStmt(*srcNodeID, *dstNodeID);
        //     break;
        // case SVFStmt::BinaryOp:
        //     pag->addBinaryOPStmt(*srcNodeID, *dstNodeID);
        //     break;
        // case SVFStmt::UnaryOp:
        //     pag->addUnaryOPStmt(*srcNodeID, *dstNodeID);
        //     break;
        // case SVFStmt::Branch:
        //     pag->addBranchStmt(*srcNodeID, *dstNodeID);
        //     break;
        // case SVFStmt::ThreadFork:
        //     pag->addThreadForkPE(*srcNodeID, *dstNodeID);
        //     break;
        // case SVFStmt::ThreadJoin:
        //     pag->addThreadJoinPE(*srcNodeID, *dstNodeID);
        //     break;
        
        default:
            break;
        }
    }
    
    return true;
}
