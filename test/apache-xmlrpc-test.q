#!/usr/bin/env qore
# -*- mode: qore; indent-tabs-mode: nil -*-

# requires apache xmlrpc-client.jar, xmlrpc-common.jar, and ws-common.jar files from
# the apache xmlrpc client in the classpath (http://ws.apache.org/xmlrpc/client.html)

# for example:
# CLASSPATH=xmlrpc-client-3.1.3.jar:xmlrpc-common-3.1.3.jar:ws-commons-util-1.0.2.jar apache-xmlrpc-test.q

# map java classes from apach jar files to qore classes
%module-cmd(gnu-java) import org.apache.xmlrpc.client.XmlRpcClientConfigImpl
%module-cmd(gnu-java) import org.apache.xmlrpc.client.XmlRpcClient
%module-cmd(gnu-java) import java.util.HashMap
%module-cmd(gnu-java) import java.net.URL

# require all variables to be declared
%require-our
%new-style
%require-types
%strict-args
%enable-all-warnings

# instantiate the getStatus2 class as the application class
%exec-class getStatus2

class getStatus2 {
    const ServerUrl = "http://localhost:8001";

    constructor() {
	# set server URL
	string server_url = ARGV[0] ?? ServerUrl;

	# create XML-RPC configuration
	XmlRpcClientConfigImpl config();

        hash uh = parse_url(server_url);
        if (uh.username)
            config.setBasicUserName(uh.username);
        if (uh.password)
            config.setBasicPassword(uh.password);

	# create URL object
	#java::net::URL url(server_url);
        URL url(server_url);
	printf("url: %s\n", url.toString());

	# set URL in config
	config.setServerURL(url);

	# create XML-RPC client oject
	XmlRpcClient client();

	# apply config (URL) to client object
	client.setConfig(config);

	# call the method on the Qorus server and get the result
	HashMap result = client.execute("omq.system.get-status", ());

	# get fields from the result
	string instance_key = result.get("instance-key");
	int sessionid = result.get("session-id");
	string version = result.get("omq-version");

	# output the result
	printf("Qorus " + version + " " + instance_key + " sessionid " + sessionid + "\n");
    }
}
