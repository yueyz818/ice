// **********************************************************************
//
// Copyright (c) 2001
// Mutable Realms, Inc.
// Huntsville, AL, USA
//
// All Rights Reserved
//
// **********************************************************************

#include <Ice/Ice.h>
#include <IcePack/ServerDeployerI.h>
#include <IcePack/ServerFactory.h>
#include <IcePack/ServerBuilder.h>
#include <IcePack/TraceLevels.h>

using namespace std;
using namespace IcePack;

IcePack::ServerDeployerI::ServerDeployerI(const NodeInfoPtr& nodeInfo) :
    _nodeInfo(nodeInfo)
{
}

IcePack::ServerDeployerI::~ServerDeployerI()
{
}

void
IcePack::ServerDeployerI::add(const string& name, const string& descriptor, const string& binPath, 
			      const string& libPath, const Targets& targets, const Ice::Current&)
{
    try
    {
	map<string, string> variables;
	variables["name"] = name;
	variables["binpath"] = binPath;
	variables["libpath"] = libPath;
	
	//
	// Component path is used to identify the component. For example:
	// node1.server1.service3
	//
	string componentPath = _nodeInfo->getNode()->getName() + "." + name;

	ServerBuilder builder(_nodeInfo, variables, componentPath, targets);

	//
	// Parse the server deployment descriptors.
	//
	builder.parse(descriptor);

	//
	// Execute the builder tasks.
	//
	builder.execute();
    }
    catch(const Ice::LocalException& ex)
    {
	cout << "uknown local exception: " << ex << endl;
    }
}

void
IcePack::ServerDeployerI::remove(const string& name, const Ice::Current&)
{
    ServerRegistryPrx registry = _nodeInfo->getServerRegistry();

    ServerPrx server;
    try
    {
	server = registry->findByName(name);
    }
    catch(const ServerNotExistException&)
    {
	ServerDeploymentException ex;
	ex.server = name;
	ex.reason = "server doesn't exist";
	throw ex;
    }

    
    ServerDescription desc;
    try
    {
	desc = server->getServerDescription();
    }
    catch(const Ice::ObjectNotExistException&)
    {
	ServerDeploymentException ex;
	ex.server = name;
	ex.reason = "server doesn't exist";
	throw ex;
    }

    //
    // Ensure the server is from this node.
    //
    if(desc.node != _nodeInfo->getNode()->getName())
    {
	ServerDeploymentException ex;
	ex.server = name;
	ex.reason = "server is not from this node";
	throw ex;	
    }

    map<string, string> variables;
    variables["name"] = name;
    variables["binpath"] = desc.path; // Required for parsing to succeed.
    variables["libpath"] = "";

    //
    // Component path is used to identify the component. For example:
    // node1.server1.service3
    //
    string componentPath = _nodeInfo->getNode()->getName() + "." + name;

    ServerBuilder builder(_nodeInfo, variables, componentPath, desc.targets);

    //
    // Parse the server deployment descriptors.
    //
    builder.parse(desc.descriptor);

    //
    // Execute the builder tasks.
    //
    builder.undo();
}
