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
 *
 * @param fileName the file the SVFIR write to
 * 
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

        // create node jsonObj
        cJSON* nodeJSONObject = cJSON_CreateObject();
        cJSON_AddNumberToObject(nodeJSONObject, "nodeID", nodeID);
        cJSON_AddNumberToObject(nodeJSONObject, "nodeKindID", nodeKindID);
        cJSON_AddStringToObject(nodeJSONObject, "nodeKindStr", nodeKindStr.c_str());
        // cJSON_AddBoolToObject(nodeJSONObject, "isIsolated", node->isIsolatedNode());
        // add fields for specific kinds of nodes
        if(nodeKindID==SVFVar::PNODEK::GepValNode){
            cJSON_AddNumberToObject(nodeJSONObject, "locationSetFldIdx", ((GepValVar*)node)->getConstantFieldIdx());
            cJSON_AddNumberToObject(nodeJSONObject, "typeID", ((GepValVar*)node)->getType()->getTypeID());
            NodeID baseNodeID = getBaseValVar(getValueNode(node->getValue()));
            cJSON_AddNumberToObject(nodeJSONObject, "baseNodeID", baseNodeID);
            // get the CurInst of this GepValNode
            const Value* curInst = nullptr;
            for(GepValueVarMap::iterator iter = GepValObjMap.begin(); iter!=GepValObjMap.end(); iter++){
                NodeLocationSetMap lsToNodeIDMap = iter->second;
                if(lsToNodeIDMap.count(std::make_pair(baseNodeID, ((GepValVar*)node)->getConstantFieldIdx()))&&lsToNodeIDMap[std::make_pair(baseNodeID, ((GepValVar*)node)->getConstantFieldIdx())]==nodeID){
                    curInst = iter->first;
                }
            }
            NodeID curInstNodeID = -1;
            if(curInst!=nullptr){
                curInstNodeID = pag->getValueNode(curInst);
            }
            cJSON_AddNumberToObject(nodeJSONObject, "curInstNodeID", curInstNodeID);
        } else if(nodeKindID==SVFVar::PNODEK::GepObjNode){
            NodeID valueNodeID = getValueNode(node->getValue());
            cJSON_AddNumberToObject(nodeJSONObject, "ValueNodeID", valueNodeID);
        }
        // add node to the nodes JSON array
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
            SVFStmt* stmt = *edgeIter;

            NodeID srcNodeID = stmt->getSrcID();
            NodeID dstNodeID = stmt->getDstID();

            // add edge to the JSON array
            cJSON *edgeJSONObject = cJSON_CreateObject();
            cJSON_AddNumberToObject(edgeJSONObject, "edgeKindID", edgeKindID);
            cJSON_AddStringToObject(edgeJSONObject, "edgeKindStr", edgeKindStr.c_str());
            cJSON_AddNumberToObject(edgeJSONObject, "srcNodeID", srcNodeID);
            cJSON_AddNumberToObject(edgeJSONObject, "dstNodeID", dstNodeID);
            
            // add fields for specific kinds of edges
            if(edgeKindID == SVFStmt::Store){
                // Problem: will the getICFGNode get the right inst?
                cJSON_AddNumberToObject(edgeJSONObject, "ICFGNodeID", stmt->getICFGNode()->getId());
            }else if(edgeKindID == SVFStmt::Call){
                cJSON_AddNumberToObject(edgeJSONObject, "CallICFGNodeID", ((CallPE*)stmt)->getCallSite()->getId());
                cJSON_AddNumberToObject(edgeJSONObject, "FunEntryICFGNodeID", ((CallPE*)stmt)->getFunEntryICFGNode()->getId());
            }else if(edgeKindID == SVFStmt::Ret){
                cJSON_AddNumberToObject(edgeJSONObject, "CallICFGNodeID", ((RetPE*)stmt)->getCallSite()->getId());
                cJSON_AddNumberToObject(edgeJSONObject, "FunExitICFGNodeID", ((RetPE*)stmt)->getFunExitICFGNode()->getId());
            }else if(edgeKindID == SVFStmt::Gep){
                // Problem: what is the difference between accumulateConstantFieldIdx() and accumulateConstantOffset()
                cJSON_AddNumberToObject(edgeJSONObject, "LocationSet", ((GepStmt*)stmt)->getLocationSet().accumulateConstantFieldIdx());
                cJSON_AddBoolToObject(edgeJSONObject, "isConstantOffset", ((GepStmt*)stmt)->isConstantOffset());
            }else if(edgeKindID == SVFStmt::Phi){
                // Problem: there is no getPredICFGNode() func
                cJSON_AddNumberToObject(edgeJSONObject, "ICFGNodeID", ((PhiStmt*)stmt)->getICFGNode()->getId());
            }else if(edgeKindID == SVFStmt::Select){
                cJSON_AddNumberToObject(edgeJSONObject, "ResID", ((SelectStmt*)stmt)->getResID());
                std::vector<SVFVar*> opnds = ((SelectStmt*)stmt)->getOpndVars();
                cJSON_AddNumberToObject(edgeJSONObject, "Op1NodeID", opnds[0]->getId());
                cJSON_AddNumberToObject(edgeJSONObject, "Op2NodeID", opnds[1]->getId());
                cJSON_AddNumberToObject(edgeJSONObject, "ConditionNodeID", ((SelectStmt*)stmt)->getCondition()->getId());
            }else if(edgeKindID == SVFStmt::Cmp){
                std::vector<SVFVar*> opnds = ((CmpStmt*)stmt)->getOpndVars();
                cJSON_AddNumberToObject(edgeJSONObject, "Op1NodeID", opnds[0]->getId());
                cJSON_AddNumberToObject(edgeJSONObject, "Op2NodeID", opnds[1]->getId());
                cJSON_AddNumberToObject(edgeJSONObject, "DstNodeID", ((CmpStmt*)stmt)->getResID());
                cJSON_AddNumberToObject(edgeJSONObject, "Predicate", ((CmpStmt*)stmt)->getPredicate());
            }else if(edgeKindID == SVFStmt::BinaryOp){
                std::vector<SVFVar*> opnds = ((BinaryOPStmt*)stmt)->getOpndVars();
                cJSON_AddNumberToObject(edgeJSONObject, "Op1NodeID", opnds[0]->getId());
                cJSON_AddNumberToObject(edgeJSONObject, "Op2NodeID", opnds[1]->getId());
                cJSON_AddNumberToObject(edgeJSONObject, "DstNodeID", ((BinaryOPStmt*)stmt)->getResID());
                cJSON_AddNumberToObject(edgeJSONObject, "Opcode", ((BinaryOPStmt*)stmt)->getOpcode());
            }else if(edgeKindID == SVFStmt::UnaryOp){
                cJSON_AddNumberToObject(edgeJSONObject, "SrcNodeID", ((UnaryOPStmt*)stmt)->getOpVarID());
                cJSON_AddNumberToObject(edgeJSONObject, "DstNodeID", ((UnaryOPStmt*)stmt)->getResID());
                cJSON_AddNumberToObject(edgeJSONObject, "Opcode", ((UnaryOPStmt*)stmt)->getOpcode());
            }else if(edgeKindID == SVFStmt::Branch){
                cJSON_AddNumberToObject(edgeJSONObject, "BranchNodeID", ((BranchStmt*)stmt)->getBranchInst()->getId());
                cJSON_AddNumberToObject(edgeJSONObject, "ConditionNodeID", ((BranchStmt*)stmt)->getCondition()->getId());
                int successorSize = ((BranchStmt*)stmt)->getNumSuccessors();
                // successors
                cJSON *successorsJSONArray = cJSON_CreateArray();
                for(int sId = 0; sId < successorSize; sId ++){
                    cJSON *successorsObj = cJSON_CreateObject();
                    cJSON_AddNumberToObject(successorsObj, "ICFGNodeID", ((BranchStmt*)stmt)->getSuccessor(sId)->getId());
                    cJSON_AddNumberToObject(successorsObj, "ConditionValue", ((BranchStmt*)stmt)->getSuccessorCondValue(sId));
                    cJSON_AddItemToArray(successorsJSONArray, successorsObj);
                }
                cJSON_AddItemToObject(edgeJSONObject, "successors", successorsJSONArray);
            }else if(edgeKindID == SVFStmt::ThreadFork){
                cJSON_AddNumberToObject(edgeJSONObject, "SrcNodeID", ((TDForkPE*)stmt)->getSrcID());
                // cJSON_AddNumberToObject(edgeJSONObject, "DstNodeID", ((TDForkPE*)stmt)->get());
                // cJSON_AddNumberToObject(edgeJSONObject, "CallICFGNodeID", ((TDForkPE*)stmt)->getCallSite()->getId());
                // cJSON_AddNumberToObject(edgeJSONObject, "FunEntryICFGNodeID", ((TDForkPE*)stmt)->getFunEntryICFGNode()->getId());
            }else if(edgeKindID == SVFStmt::ThreadFork){
                // cJSON_AddNumberToObject(edgeJSONObject, "SrcNodeID", ((TDJoinPE*)stmt)->getSrcID());
                // cJSON_AddNumberToObject(edgeJSONObject, "DstNodeID", ((TDJoinPE*)stmt)->get());
                // cJSON_AddNumberToObject(edgeJSONObject, "CallICFGNodeID", ((TDJoinPE*)stmt)->getCallSite()->getId());
                // cJSON_AddNumberToObject(edgeJSONObject, "FunEntryICFGNodeID", ((TDJoinPE*)stmt)->getFunEntryICFGNode()->getId());
            }
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
 *
 * @param fileName the file the SVFIR read from
 * 
 */
bool SVFIR::readFromFile(std::string fileName, SVFModule* svfModule){
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

    // prepare the nodeID to node map based on the SVF module
    Map<NodeID, const Value*> NodeIDToValueMap;
    // traverse the symTable and get four kinds of node: valNode, objNode, retNode, varargNode
    SymbolTableInfo* symTable = SymbolTableInfo::SymbolInfo();
    for (SymbolTableInfo::ValueToIDMapTy::iterator iter =
                symTable->valSyms().begin(); iter != symTable->valSyms().end();
            ++iter)
    {
        if(iter->second == symTable->blkPtrSymID() || iter->second == symTable->nullPtrSymID())
            continue;
        NodeIDToValueMap[(NodeID)iter->second]=iter->first;
    }
    for (SymbolTableInfo::ValueToIDMapTy::iterator iter =
                symTable->objSyms().begin(); iter != symTable->objSyms().end();
            ++iter)
    {
        if(iter->second == symTable->blackholeSymID() || iter->second == symTable->constantSymID())
            continue;
        NodeIDToValueMap[(NodeID)iter->second]=iter->first;
    }
    for (SymbolTableInfo::FunToIDMapTy::iterator iter =
                symTable->retSyms().begin(); iter != symTable->retSyms().end();
            ++iter)
    {
        const SVFFunction* fun = LLVMModuleSet::getLLVMModuleSet()->getSVFFunction(iter->first);
        NodeIDToValueMap[(NodeID)iter->second]=(const Value*)fun;
    }
    for (SymbolTableInfo::FunToIDMapTy::iterator iter =
                symTable->varargSyms().begin();
            iter != symTable->varargSyms().end(); ++iter)
    {
        const SVFFunction* fun = LLVMModuleSet::getLLVMModuleSet()->getSVFFunction(iter->first);
        NodeIDToValueMap[(NodeID)iter->second]=(const Value*)fun;
    }

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
        NodeID newNodeID = 0;
        assert( SVFVar::PNODEK::ValNode <= nodeKindID && nodeKindID <= SVFVar::PNODEK::DummyObjNode && "new SVFIR node kind?");
        switch (nodeKindID)
        {
        case SVFVar::PNODEK::ValNode:
        {
            assert(NodeIDToValueMap.find(nodeID)!=NodeIDToValueMap.end()&&"cannot find the value for a ValNode!");
            pag->addValNode(NodeIDToValueMap.find(nodeID)->second, nodeID);
            break;
        }
        case SVFVar::PNODEK::RetNode:
        {
            assert(NodeIDToValueMap.find(nodeID)!=NodeIDToValueMap.end()&&"cannot find the value for a RetNode!");
            pag->addValNode(NodeIDToValueMap.find(nodeID)->second, nodeID);
            break;
        }
        case SVFVar::PNODEK::VarargNode:
        {
            assert(NodeIDToValueMap.find(nodeID)!=NodeIDToValueMap.end()&&"cannot find the value for a VarargNode!");
            pag->addValNode(NodeIDToValueMap.find(nodeID)->second, nodeID);
            break;
        }
        case SVFVar::PNODEK::ObjNode:
        {
            // Problem: func addObjNode will call addFIObjNode, so there will be not any objNode?
            assert(NodeIDToValueMap.find(nodeID)!=NodeIDToValueMap.end()&&"cannot find the value for a ObjNode!");
            pag->addObjNode(NodeIDToValueMap.find(nodeID)->second, nodeID);
            break;
        }
        case SVFVar::PNODEK::GepValNode:
        {
            // parse LocationSet
            const cJSON* SVFIRJsonObjNodeLocationSetFldIdx = cJSON_GetObjectItem(SVFIRJsonObjNode, "locationSetFldIdx");
            assert(SVFIRJsonObjNodeLocationSetFldIdx && cJSON_IsNumber(SVFIRJsonObjNodeLocationSetFldIdx) && "Parse SVFIR locationSetFldIdx failed.");
            LocationSet ls = LocationSet(SVFIRJsonObjNodeLocationSetFldIdx->valueint);

            // parse Type
            const cJSON* SVFIRJsonObjNodeTypeID = cJSON_GetObjectItem(SVFIRJsonObjNode, "typeID");
            assert(SVFIRJsonObjNodeTypeID && cJSON_IsNumber(SVFIRJsonObjNodeTypeID) && "Parse SVFIR typeID failed.");
            // Problem: donot know how to create a Type.
            // Type ty = Type(LLVMModuleSet::getLLVMModuleSet()->getContext(), SVFIRJsonObjNodeTypeID->valueint);

            // parse baseNodeID
            const cJSON* SVFIRJsonObjNodeBaseNodeID = cJSON_GetObjectItem(SVFIRJsonObjNode, "baseNodeID");
            assert(SVFIRJsonObjNodeBaseNodeID && cJSON_IsNumber(SVFIRJsonObjNodeBaseNodeID) && "Parse SVFIR baseNodeID failed.");

            // parse curInstNodeID
            const cJSON* SVFIRJsonObjNodeCurInstNodeID = cJSON_GetObjectItem(SVFIRJsonObjNode, "curInstNodeID");
            assert(SVFIRJsonObjNodeCurInstNodeID && cJSON_IsNumber(SVFIRJsonObjNodeCurInstNodeID) && "Parse SVFIR curInstNodeID failed.");
            
            // search values by NodeID
            assert( (NodeIDToValueMap.find(SVFIRJsonObjNodeCurInstNodeID->valueint)!=NodeIDToValueMap.end()) && "cannot find the value by SVFIRJsonObjNodeCurInstNodeID!");
            const Value* curInst = NodeIDToValueMap.find(SVFIRJsonObjNodeCurInstNodeID->valueint)->second;
            assert(NodeIDToValueMap.find(SVFIRJsonObjNodeBaseNodeID->valueint)!=NodeIDToValueMap.end()&&"cannot find the value by SVFIRJsonObjNodeBaseNodeID!");
            const Value* baseNode = NodeIDToValueMap.find(SVFIRJsonObjNodeBaseNodeID->valueint)->second;

            pag->addGepValNode(curInst, baseNode, ls, nodeID, nullptr);
            break;
        }
        case SVFVar::PNODEK::DummyValNode:
        {
            // Problem: cannot set the NodeID for a new dummy node, so map the new generated ID to the old NodeID loaded from the IR file.
            newNodeID = pag->addDummyValNode();
            nodeIDtoNewIDMap[newNodeID] = nodeID;
            break;
        }
        case SVFVar::PNODEK::GepObjNode:
        {
            // Problem: GepObjNodes are added when SVF analyze the PAG, not when SVF build PAG. And the MemObj is created before?
            const cJSON* SVFIRJsonObjNodeValueNodeID = cJSON_GetObjectItem(SVFIRJsonObjNode, "ValueNodeID");
            assert(SVFIRJsonObjNodeValueNodeID && cJSON_IsNumber(SVFIRJsonObjNodeValueNodeID) && "Parse SVFIR ValueNodeID failed.");
            const Value* value = NodeIDToValueMap.find(SVFIRJsonObjNodeValueNodeID->valueint)->second;
            // TODO: check the cast
            const GepObjVar* gepNode = (GepObjVar*)value;
            pag->addGepObjNode(gepNode->getMemObj(), nodeID);
            break;
        }
        case SVFVar::PNODEK::FIObjNode:
        {
            assert(NodeIDToValueMap.find(nodeID)!=NodeIDToValueMap.end()&&"cannot find the value for a FIObjNode!");
            // Problem: not sure is it right
            newNodeID = pag->addFIObjNode(IRGraph::getMemObj(NodeIDToValueMap.find(nodeID)->second));
            nodeIDtoNewIDMap[newNodeID] = nodeID;
            break;
        }
        case SVFVar::PNODEK::DummyObjNode:
        {
            // Problem: donot know how to create a Type. 
            newNodeID = pag->addDummyObjNode(nullptr);
            nodeIDtoNewIDMap[newNodeID] = nodeID;
            break;
        }
        
        default:
            assert("new SVFIR node kind?");
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

        assert( SVFStmt::Addr <= edgeKindID && edgeKindID <= SVFStmt::ThreadJoin && "new SVFIR edge kind?");
        switch (edgeKindID)
        {
        case SVFStmt::Addr:
        {
            pag->addAddrStmt(*srcNodeID, *dstNodeID);
            break;
        }
        case SVFStmt::Copy:
        {
            pag->addCopyStmt(*srcNodeID, *dstNodeID);
            break;
        }
        case SVFStmt::Store:
        {
            // Problem: Did the ICFG build before?
            const cJSON* SVFIRJsonObjEdgeICFGNodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "ICFGNodeID");
            assert(SVFIRJsonObjEdgeICFGNodeID && cJSON_IsNumber(SVFIRJsonObjEdgeICFGNodeID) && "Parse SVFIR edge ICFGNodeID failed.");
            assert(NodeIDToValueMap.find(SVFIRJsonObjEdgeICFGNodeID->valueint)!=NodeIDToValueMap.end()&&"cannot find the ICFGNode for a store edge!");
            // Problem: is it right?
            const Instruction* inst = SVFUtil::dyn_cast<Instruction>(NodeIDToValueMap.find(SVFIRJsonObjEdgeICFGNodeID->valueint)->second);
            IntraICFGNode* node = pag->getICFG()->getIntraICFGNode(inst);

            pag->addStoreStmt(*srcNodeID, *dstNodeID, node);
            break;
        }
        case SVFStmt::Load:
        {
            pag->addLoadStmt(*srcNodeID, *dstNodeID);
            break;
        }
        case SVFStmt::Call:
        {
            const cJSON* SVFIRJsonObjEdgeCallICFGNodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "CallICFGNodeID");
            assert(SVFIRJsonObjEdgeCallICFGNodeID && cJSON_IsNumber(SVFIRJsonObjEdgeCallICFGNodeID) && "Parse SVFIR edge CallICFGNodeID failed.");
            const cJSON* SVFIRJsonObjEdgeFunEntryICFGNodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "FunEntryICFGNodeID");
            assert(SVFIRJsonObjEdgeFunEntryICFGNodeID && cJSON_IsNumber(SVFIRJsonObjEdgeFunEntryICFGNodeID) && "Parse SVFIR edge FunEntryICFGNodeID failed.");
            // Problem: is it right? use the func getGNode()
            CallICFGNode* callNode = (CallICFGNode*) pag->getICFG()->getGNode(SVFIRJsonObjEdgeCallICFGNodeID->valueint);
            FunEntryICFGNode* FunEntryNode = (FunEntryICFGNode*) pag->getICFG()->getGNode(SVFIRJsonObjEdgeFunEntryICFGNodeID->valueint);
            pag->addCallPE(*srcNodeID, *dstNodeID, callNode, FunEntryNode);
            break;
        }
        case SVFStmt::Ret:
        {
            const cJSON* SVFIRJsonObjEdgeCallICFGNodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "CallICFGNodeID");
            assert(SVFIRJsonObjEdgeCallICFGNodeID && cJSON_IsNumber(SVFIRJsonObjEdgeCallICFGNodeID) && "Parse SVFIR edge CallICFGNodeID failed.");
            const cJSON* SVFIRJsonObjEdgeFunExitICFGNodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "FunExitICFGNodeID");
            assert(SVFIRJsonObjEdgeFunExitICFGNodeID && cJSON_IsNumber(SVFIRJsonObjEdgeFunExitICFGNodeID) && "Parse SVFIR edge SVFIRJsonObjEdgeFunExitICFGNodeID failed.");
            CallICFGNode* callNode = (CallICFGNode*) pag->getICFG()->getGNode(SVFIRJsonObjEdgeCallICFGNodeID->valueint);
            FunExitICFGNode* FunExitNode = (FunExitICFGNode*) pag->getICFG()->getGNode(SVFIRJsonObjEdgeFunExitICFGNodeID->valueint);
            pag->addRetPE(*srcNodeID, *dstNodeID, callNode, FunExitNode);
            break;
        }
        case SVFStmt::Gep:
        {
            const cJSON* SVFIRJsonObjEdgeLocationSet = cJSON_GetObjectItem(SVFIRJsonObjEdge, "LocationSet");
            assert(SVFIRJsonObjEdgeLocationSet && cJSON_IsNumber(SVFIRJsonObjEdgeLocationSet) && "Parse SVFIR edge LocationSet failed.");
            const cJSON* SVFIRJsonObjEdgeIsConstantOffset = cJSON_GetObjectItem(SVFIRJsonObjEdge, "isConstantOffset");
            assert(SVFIRJsonObjEdgeIsConstantOffset && cJSON_IsBool(SVFIRJsonObjEdgeIsConstantOffset) && "Parse SVFIR edge isConstantOffset failed.");
            pag->addGepStmt(*srcNodeID, *dstNodeID, LocationSet(SVFIRJsonObjEdgeLocationSet->valueint), SVFIRJsonObjEdgeIsConstantOffset->type);
            break;
        }
        case SVFStmt::Phi:
        {
            const cJSON* SVFIRJsonObjEdgeICFGNodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "ICFGNodeID");
            assert(SVFIRJsonObjEdgeICFGNodeID && cJSON_IsNumber(SVFIRJsonObjEdgeICFGNodeID) && "Parse SVFIR edge ICFGNodeID failed.");
            ICFGNode* icfgNode = pag->getICFG()->getGNode(SVFIRJsonObjEdgeICFGNodeID->valueint);
            pag->addPhiStmt(*srcNodeID, *dstNodeID, icfgNode);
            break;
        }
        case SVFStmt::Select:
        {
            // get res NodeID
            const cJSON* SVFIRJsonObjEdgeResID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "ResID");
            assert(SVFIRJsonObjEdgeResID && cJSON_IsNumber(SVFIRJsonObjEdgeResID) && "Parse SVFIR edge ResID failed.");
            assert(NodeIDToValueMap.find(SVFIRJsonObjEdgeResID->valueint)!=NodeIDToValueMap.end()&&"cannot find the ResID for a Select edge!");
            NodeID resNodeID = SVFIRJsonObjEdgeResID->valueint;
            // const Value* resNode = NodeIDToValueMap.find(SVFIRJsonObjEdgeResID->valueint)->second;

            // get op1 NodeID
            const cJSON* SVFIRJsonObjEdgeOp1NodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "Op1NodeID");
            assert(SVFIRJsonObjEdgeOp1NodeID && cJSON_IsNumber(SVFIRJsonObjEdgeOp1NodeID) && "Parse SVFIR edge Op1NodeID failed.");
            assert(NodeIDToValueMap.find(SVFIRJsonObjEdgeOp1NodeID->valueint)!=NodeIDToValueMap.end()&&"cannot find the Op1NodeID for a Select edge!");
            NodeID op1NodeID = SVFIRJsonObjEdgeOp1NodeID->valueint;
            // const Value* op1Node = NodeIDToValueMap.find(SVFIRJsonObjEdgeOp1NodeID->valueint)->second;

            // get op2 NodeID
            const cJSON* SVFIRJsonObjEdgeOp2NodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "Op2NodeID");
            assert(SVFIRJsonObjEdgeOp2NodeID && cJSON_IsNumber(SVFIRJsonObjEdgeOp2NodeID) && "Parse SVFIR edge Op2NodeID failed.");
            assert(NodeIDToValueMap.find(SVFIRJsonObjEdgeOp2NodeID->valueint)!=NodeIDToValueMap.end()&&"cannot find the Op2NodeID for a Select edge!");
            NodeID op2NodeID = SVFIRJsonObjEdgeOp2NodeID->valueint;
            // const Value* op2Node = NodeIDToValueMap.find(SVFIRJsonObjEdgeOp2NodeID->valueint)->second;

            // get condition NodeID
            const cJSON* SVFIRJsonObjEdgeConditionNodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "ConditionNodeID");
            assert(SVFIRJsonObjEdgeConditionNodeID && cJSON_IsNumber(SVFIRJsonObjEdgeConditionNodeID) && "Parse SVFIR edge ConditionNodeID failed.");
            assert(NodeIDToValueMap.find(SVFIRJsonObjEdgeConditionNodeID->valueint)!=NodeIDToValueMap.end()&&"cannot find the ConditionNodeID for a Select edge!");
            NodeID conditionNodeID = SVFIRJsonObjEdgeConditionNodeID->valueint;
            // const Value* conditionNode = NodeIDToValueMap.find(SVFIRJsonObjEdgeConditionNodeID->valueint)->second;
            
            pag->addSelectStmt(resNodeID, op1NodeID, op2NodeID, conditionNodeID);
            break;
        }
        case SVFStmt::Cmp:
        {
            // get dst NodeID
            const cJSON* SVFIRJsonObjEdgeDstNodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "DstNodeID");
            assert(SVFIRJsonObjEdgeDstNodeID && cJSON_IsNumber(SVFIRJsonObjEdgeDstNodeID) && "Parse SVFIR edge DstNodeID failed.");
            assert(NodeIDToValueMap.find(SVFIRJsonObjEdgeDstNodeID->valueint)!=NodeIDToValueMap.end()&&"cannot find the DstNodeID for a Cmp edge!");
            NodeID dstNodeID = SVFIRJsonObjEdgeDstNodeID->valueint;

            // get op1 NodeID
            const cJSON* SVFIRJsonObjEdgeOp1NodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "Op1NodeID");
            assert(SVFIRJsonObjEdgeOp1NodeID && cJSON_IsNumber(SVFIRJsonObjEdgeOp1NodeID) && "Parse SVFIR edge Op1NodeID failed.");
            assert(NodeIDToValueMap.find(SVFIRJsonObjEdgeOp1NodeID->valueint)!=NodeIDToValueMap.end()&&"cannot find the Op1NodeID for a Cmp edge!");
            NodeID op1NodeID = SVFIRJsonObjEdgeOp1NodeID->valueint;

            // get op2 NodeID
            const cJSON* SVFIRJsonObjEdgeOp2NodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "Op2NodeID");
            assert(SVFIRJsonObjEdgeOp2NodeID && cJSON_IsNumber(SVFIRJsonObjEdgeOp2NodeID) && "Parse SVFIR edge Op2NodeID failed.");
            assert(NodeIDToValueMap.find(SVFIRJsonObjEdgeOp2NodeID->valueint)!=NodeIDToValueMap.end()&&"cannot find the Op2NodeID for a Cmp edge!");
            NodeID op2NodeID = SVFIRJsonObjEdgeOp2NodeID->valueint;

            // get Predicate node
            const cJSON* SVFIRJsonObjEdgePredicate = cJSON_GetObjectItem(SVFIRJsonObjEdge, "Predicate");
            assert(SVFIRJsonObjEdgePredicate && cJSON_IsNumber(SVFIRJsonObjEdgePredicate) && "Parse SVFIR edge Predicate failed.");

            pag->addCmpStmt(op1NodeID, op2NodeID, dstNodeID, SVFIRJsonObjEdgePredicate->valueint);
            break;
        }
        case SVFStmt::BinaryOp:
        {
            // get dst NodeID
            const cJSON* SVFIRJsonObjEdgeDstNodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "DstNodeID");
            assert(SVFIRJsonObjEdgeDstNodeID && cJSON_IsNumber(SVFIRJsonObjEdgeDstNodeID) && "Parse SVFIR edge DstNodeID failed.");
            assert(NodeIDToValueMap.find(SVFIRJsonObjEdgeDstNodeID->valueint)!=NodeIDToValueMap.end()&&"cannot find the DstNodeID for a BinaryOp edge!");
            NodeID dstNodeID = SVFIRJsonObjEdgeDstNodeID->valueint;

            // get op1 NodeID
            const cJSON* SVFIRJsonObjEdgeOp1NodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "Op1NodeID");
            assert(SVFIRJsonObjEdgeOp1NodeID && cJSON_IsNumber(SVFIRJsonObjEdgeOp1NodeID) && "Parse SVFIR edge Op1NodeID failed.");
            assert(NodeIDToValueMap.find(SVFIRJsonObjEdgeOp1NodeID->valueint)!=NodeIDToValueMap.end()&&"cannot find the Op1NodeID for a BinaryOp edge!");
            NodeID op1NodeID = SVFIRJsonObjEdgeOp1NodeID->valueint;

            // get op2 NodeID
            const cJSON* SVFIRJsonObjEdgeOp2NodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "Op2NodeID");
            assert(SVFIRJsonObjEdgeOp2NodeID && cJSON_IsNumber(SVFIRJsonObjEdgeOp2NodeID) && "Parse SVFIR edge Op2NodeID failed.");
            assert(NodeIDToValueMap.find(SVFIRJsonObjEdgeOp2NodeID->valueint)!=NodeIDToValueMap.end()&&"cannot find the Op2NodeID for a BinaryOp edge!");
            NodeID op2NodeID = SVFIRJsonObjEdgeOp2NodeID->valueint;

            // get op code
            const cJSON* SVFIRJsonObjEdgeOpcode = cJSON_GetObjectItem(SVFIRJsonObjEdge, "Opcode");
            assert(SVFIRJsonObjEdgeOpcode && cJSON_IsNumber(SVFIRJsonObjEdgeOpcode) && "Parse SVFIR edge BinaryOp opcode failed.");

            pag->addBinaryOPStmt(op1NodeID, op2NodeID, dstNodeID, SVFIRJsonObjEdgeOpcode->valueint);
            break;
        }
        case SVFStmt::UnaryOp:
        {
            // get dst NodeID
            const cJSON* SVFIRJsonObjEdgeSrcNodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "SrcNodeID");
            assert(SVFIRJsonObjEdgeSrcNodeID && cJSON_IsNumber(SVFIRJsonObjEdgeSrcNodeID) && "Parse SVFIR edge DstNodeID failed.");
            assert(NodeIDToValueMap.find(SVFIRJsonObjEdgeSrcNodeID->valueint)!=NodeIDToValueMap.end()&&"cannot find the DstNodeID for a UnaryOp edge!");
            NodeID srcNodeID = SVFIRJsonObjEdgeSrcNodeID->valueint;

            // get dst NodeID
            const cJSON* SVFIRJsonObjEdgeDstNodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "DstNodeID");
            assert(SVFIRJsonObjEdgeDstNodeID && cJSON_IsNumber(SVFIRJsonObjEdgeDstNodeID) && "Parse SVFIR edge DstNodeID failed.");
            assert(NodeIDToValueMap.find(SVFIRJsonObjEdgeDstNodeID->valueint)!=NodeIDToValueMap.end()&&"cannot find the DstNodeID for a UnaryOp edge!");
            NodeID dstNodeID = SVFIRJsonObjEdgeDstNodeID->valueint;

            // get op code
            const cJSON* SVFIRJsonObjEdgeOpcode = cJSON_GetObjectItem(SVFIRJsonObjEdge, "Opcode");
            assert(SVFIRJsonObjEdgeOpcode && cJSON_IsNumber(SVFIRJsonObjEdgeOpcode) && "Parse SVFIR edge UnaryOp opcode failed.");

            pag->addUnaryOPStmt(srcNodeID, dstNodeID, SVFIRJsonObjEdgeOpcode->valueint);
            break;
        }
        case SVFStmt::Branch:
        {
            // get branch NodeID
            const cJSON* SVFIRJsonObjEdgeBranchNodeID = cJSON_GetObjectItem(SVFIRJsonObjEdge, "BranchNodeID");
            assert(SVFIRJsonObjEdgeBranchNodeID && cJSON_IsNumber(SVFIRJsonObjEdgeBranchNodeID) && "Parse SVFIR edge BranchNodeID failed.");
            assert(NodeIDToValueMap.find(SVFIRJsonObjEdgeBranchNodeID->valueint)!=NodeIDToValueMap.end()&&"cannot find the BranchNodeID for a Branch edge!");
            NodeID branchNodeID = SVFIRJsonObjEdgeBranchNodeID->valueint;

            // get condition NodeID
            const cJSON* SVFIRJsonObjEdgeConditionValue = cJSON_GetObjectItem(SVFIRJsonObjEdge, "ConditionValue");
            assert(SVFIRJsonObjEdgeConditionValue && cJSON_IsNumber(SVFIRJsonObjEdgeConditionValue) && "Parse SVFIR edge ConditionValue failed.");
            assert(NodeIDToValueMap.find(SVFIRJsonObjEdgeConditionValue->valueint)!=NodeIDToValueMap.end()&&"cannot find the ConditionValue for a Branch edge!");
            NodeID conditionNodeID = SVFIRJsonObjEdgeConditionValue->valueint;
            
            // get the successors
            // parse successors array
            const cJSON* SVFIRJsonObjEdgeSuccessors = cJSON_GetObjectItem(SVFIRJsonObj, "successors");
            assert(SVFIRJsonObjEdgeSuccessors && cJSON_IsArray(SVFIRJsonObjEdgeSuccessors) && "Parse SVFIR successors failed.");
            const cJSON* successorJsonObj = NULL;
            BranchStmt::SuccAndCondPairVec successors;
            cJSON_ArrayForEach(successorJsonObj, SVFIRJsonObjEdgeSuccessors){
                const cJSON* icfgNodeID = cJSON_GetObjectItem(successorJsonObj, "ICFGNodeID");
                assert(icfgNodeID && cJSON_IsNumber(icfgNodeID) && "Parse SVFIR successors icfgNodeID failed.");
                const ICFGNode* icfgNode = pag->getICFG()->getGNode(icfgNodeID->valueint);
                const cJSON* conditionValue = cJSON_GetObjectItem(successorJsonObj, "ConditionValue");
                assert(conditionValue && cJSON_IsNumber(conditionValue) && "Parse SVFIR successors conditionValue failed.");
                successors.push_back(std::make_pair(icfgNode, conditionValue->valueint));
            }
            pag->addBranchStmt(branchNodeID, conditionNodeID, successors);
            break;
        }
        // case SVFStmt::ThreadFork:
        // {
        //     pag->addThreadForkPE(*srcNodeID, *dstNodeID);
        //     break;
        // }
        // case SVFStmt::ThreadJoin:
        // {
        //     pag->addThreadJoinPE(*srcNodeID, *dstNodeID);
        //     break;
        // }
        
        default:
        {
            assert("new SVFIR edge kind?");
            break;

        }
        }
    }
    
    return true;
}
