//===- svf-ex.cpp -- A driver example of SVF-------------------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013->  <Yulei Sui>
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
//===-----------------------------------------------------------------------===//

/*
 // A driver program of SVF including usages of SVF APIs
 //
 // Author: Yulei Sui,
 */

#include "SVF-LLVM/LLVMUtil.h"
#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "Util/Options.h"

#include "SABER/SrcSnkDDA.h"

using namespace llvm;
using namespace std;
using namespace SVF;

/*!
 * An example to query alias results of two LLVM values
 */
SVF::AliasResult aliasQuery(PointerAnalysis* pta, Value* v1, Value* v2)
{
    SVFValue* val1 = LLVMModuleSet::getLLVMModuleSet()->getSVFValue(v1);
    SVFValue* val2 = LLVMModuleSet::getLLVMModuleSet()->getSVFValue(v2);

    return pta->alias(val1,val2);
}

/*!
 * An example to print points-to set of an LLVM value
 */
std::string printPts(PointerAnalysis* pta, Value* val)
{

    std::string str;
    raw_string_ostream rawstr(str);
    SVFValue* svfval = LLVMModuleSet::getLLVMModuleSet()->getSVFValue(val);

    NodeID pNodeId = pta->getPAG()->getValueNode(svfval);
    const PointsTo& pts = pta->getPts(pNodeId);
    for (PointsTo::iterator ii = pts.begin(), ie = pts.end();
            ii != ie; ii++)
    {
        rawstr << " " << *ii << " ";
        PAGNode* targetObj = pta->getPAG()->getGNode(*ii);
        if(targetObj->hasValue())
        {
            rawstr << "(" << targetObj->getValue()->toString() << ")\t ";
        }
    }

    return rawstr.str();

}


/*!
 * An example to query/collect all successor nodes from a ICFGNode (iNode) along control-flow graph (ICFG)
 */
void traverseOnICFG(ICFG* icfg, const Instruction* inst)
{
    const ICFGNode* iNode = LLVMModuleSet::getLLVMModuleSet()->getICFGNode(inst);

    FIFOWorkList<const ICFGNode*> worklist;
    Set<const ICFGNode*> visited;
    worklist.push(iNode);

    /// Traverse along VFG
    while (!worklist.empty())
    {
        const ICFGNode* iNode = worklist.pop();
        for (ICFGNode::const_iterator it = iNode->OutEdgeBegin(), eit =
                    iNode->OutEdgeEnd(); it != eit; ++it)
        {
            ICFGEdge* edge = *it;
            ICFGNode* succNode = edge->getDstNode();
            if (visited.find(succNode) == visited.end())
            {
                visited.insert(succNode);
                worklist.push(succNode);
            }
        }
    }
}

/*!
 * An example to query/collect all the uses of a definition of a value along value-flow graph (VFG)
 */
void traverseOnVFG(const SVFG* vfg, Value* val)
{
    SVFIR* pag = SVFIR::getPAG();
    SVFValue* svfval = LLVMModuleSet::getLLVMModuleSet()->getSVFValue(val);

    PAGNode* pNode = pag->getGNode(pag->getValueNode(svfval));
    const VFGNode* vNode = vfg->getDefSVFGNode(pNode);
    FIFOWorkList<const VFGNode*> worklist;
    Set<const VFGNode*> visited;
    worklist.push(vNode);

    /// Traverse along VFG
    while (!worklist.empty())
    {
        const VFGNode* vNode = worklist.pop();
        for (VFGNode::const_iterator it = vNode->OutEdgeBegin(), eit =
                    vNode->OutEdgeEnd(); it != eit; ++it)
        {
            VFGEdge* edge = *it;
            VFGNode* succNode = edge->getDstNode();
            if (visited.find(succNode) == visited.end())
            {
                visited.insert(succNode);
                worklist.push(succNode);
            }
        }
    }

    /// Collect all LLVM Values
    for(Set<const VFGNode*>::const_iterator it = visited.begin(), eit = visited.end(); it!=eit; ++it)
    {
        const VFGNode* node = *it;
        /// can only query VFGNode involving top-level pointers (starting with % or @ in LLVM IR)
        /// PAGNode* pNode = vfg->getLHSTopLevPtr(node);
        /// Value* val = pNode->getValue();
    }
}

int main(int argc, char ** argv)
{

    std::vector<std::string> moduleNameVec;
    moduleNameVec = OptionBase::parseOptions(
            argc, argv, "Whole Program Points-to Analysis", "[options] <input-bitcode...>"
    );

    if (Options::WriteAnder() == "ir_annotator")
    {
        LLVMModuleSet::preProcessBCs(moduleNameVec);
    }

    SVFModule* svfModule = LLVMModuleSet::buildSVFModule(moduleNameVec);

    /// Build Program Assignment Graph (SVFIR)
    SVFIRBuilder builder(svfModule);
    SVFIR* pag = builder.build();

    /// Create Andersen's pointer analysis
    Andersen* ander = AndersenWaveDiff::createAndersenWaveDiff(pag);

    /// Query aliases
    /// aliasQuery(ander,value1,value2);

    /// Print points-to information
    // / printPts(ander, value1);

    /// Call Graph
    CallGraph* callgraph = ander->getCallGraph();

    /// ICFG
    ICFG* icfg = pag->getICFG();

    /// Value-Flow Graph (VFG)
    VFG* vfg = new VFG(callgraph);

    for(auto itr = icfg->begin(), eit = icfg->end(); itr != eit; ++itr)
    {
        SVF::ICFGNode* this_node = itr->second;

        // cerr << "wah" << endl;
        if(this_node->getFun() == 0)
        {
            cerr << "skipping" << endl;
            continue;
        }
        string fname = this_node->getFun()->getName();
        // SVF::ICFGNode::SVFStmtList stmts = this_node->getSVFStmts();

        //comparing getNodeKind and getType. they are different
        // cerr << this_node->getNodeKind() << "/" << this_node->getType() << endl;

        // cerr << (SVF::SVFBaseNode::GNodeK::FunCallBlock == this_node->getNodeKind() ? "true" : "false") << endl;
        if(CallICFGNode* call_node = dyn_cast<CallICFGNode>(this_node))
        {
            if (call_node->getCalledFunction()->toString().find("system") != -1)
            {
                cerr << "found sink: " << call_node->getCalledFunction()->toString() << endl;
            }
        }

        if(CallICFGNode* call_node = dyn_cast<CallICFGNode>(this_node))
        {
            if (call_node->getCalledFunction()->toString().find("taint_var") != -1)
            {
                cerr << "found source: " << call_node->getCalledFunction()->toString() << endl;
            }
        }

        // cerr << this_node->toString() << endl;
        // cerr << this_node->getType() << endl;
        // cerr << this_node->getType()->toString() << endl;
        
        // cerr << fname << endl;
        // for(auto stmt: stmts)
        // {
        //     cerr << stmt << endl;
        // }
        // cerr << endl;

        // if(fname.find("taint_var") != -1)
        // {
        //     cerr << "source: " << fname << endl;
        // }
        // if(fname.find("system") != -1)
        // {
        //     cerr << "sink: " << fname << endl;
        // }
    }

    /// Sparse value-flow graph (SVFG)
    SVFGBuilder svfBuilder;
    SVFG* svfg = svfBuilder.buildFullSVFG(ander);

    /// Collect uses of an LLVM Value
    // traverseOnVFG(svfg, value);

    /// Collect all successor nodes on ICFG
    // traverseOnICFG(icfg, value);

    // clean up memory
    delete vfg;
    AndersenWaveDiff::releaseAndersenWaveDiff();
    SVFIR::releaseSVFIR();

    // LLVMModuleSet::getLLVMModuleSet()->dumpModulesToFile(".svf.bc");
    SVF::LLVMModuleSet::releaseLLVMModuleSet();

    llvm::llvm_shutdown();
    return 0;
}

/// @brief dummy extension of SrcSnkDDA
class TaintChecker: public SrcSnkDDA
{
public:
    TaintChecker()
    {
    }
    virtual ~TaintChecker()
    {
    }

    virtual bool runOnModule(SVFIR* pag)
    {
        analyze(pag->getModule());
        return false;
    }

    virtual void initSrcs() override
    {
        PAG* pag = this->getPAG();
        ICFG* icfg = pag->getICFG();

        for(ICFG::const_iterator itr = icfg->begin(), eit = icfg->end(); itr != eit; ++itr)
        {
            const SVF::ICFGNode* this_node = itr->second;

            if(const CallICFGNode* call_node = dyn_cast<CallICFGNode>(this_node))
            {
                if (isSourceLikeFun(call_node->getCalledFunction()))
                {
                    cerr << "found source: " << call_node->getCalledFunction()->toString() << endl;

                    const RetICFGNode* retBlockNode = call_node->getRetICFGNode();
                    const PAGNode* pagNode = pag->getCallSiteRet(retBlockNode);
                    const SVFGNode* node = getSVFG()->getDefSVFGNode(pagNode);

                    this->addToSources(node);
                }
            }
        }
    }

    virtual void initSnks() override
    {
        PAG* pag = this->getPAG();
        ICFG* icfg = pag->getICFG();

        for(ICFG::const_iterator itr = icfg->begin(), eit = icfg->end(); itr != eit; ++itr)
        {
            const SVF::ICFGNode* this_node = itr->second;

            if(const CallICFGNode* call_node = dyn_cast<CallICFGNode>(this_node))
            {
                if (isSinkLikeFun(call_node->getCalledFunction()))
                {
                    cerr << "found sink: " << call_node->getCalledFunction()->toString() << endl;
                    
                    //get first argument
                    const SVFVar* sink_arg = call_node->getArgument(0);
                    //turn that arg into an SVFGNode
                    pag-> sink_arg


                    this->addToSinks();
                }
            }
        }
    }

    /// @brief is it a call to `taint_var`?
    /// @param fun the function being scrutinized
    /// @return if it is a call to `taint_var`
    virtual inline bool isSourceLikeFun(const SVFFunction* fun) override
    {
        if (fun->toString().find("taint_var") != -1)
        {
            return true;
        }
        return false;
    }

    /// @brief is it a call to `system`?
    /// @param fun the function being scrutinized
    /// @return if it is a call to `system`
    virtual inline bool isSinkLikeFun(const SVFFunction* fun) override
    {
        if (fun->toString().find("system") != -1)
        {
            return true;
        }
        return false;    
    }
};