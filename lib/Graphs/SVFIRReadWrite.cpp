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

#include "Graphs/SVFIRReadWrite.h"

/*!
 * Export SVFIR into a file
 */
void SVFIRReadWrite::exportSVFIRToFile(SVF::SVFIR svfir, std::string fileName){

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
        map<SVFVar::PNODEK, std::string> nodeKindToStrMap = {
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
        map<SVFVar::PNODEK, std::string>::iterator nodeKindToStrMapIter = nodeKindToStrMap.find((SVFVar::PNODEK)nodeKindID);
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
    map<SVFStmt::PEDGEK, std::string> edgeKindToStrMap = {
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
    for (map<SVFStmt::PEDGEK, std::string>::iterator edgeKindIter = edgeKindToStrMap.begin(), endOfEdgeKindIter = edgeKindToStrMap.end();
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
    error_code err;
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