#!/usr/bin/env qore
# -*- mode: qore; indent-tabs-mode: nil -*-

# requires apache xmlrpc-client.jar, xmlrpc-common.jar, and ws-common.jar files from 
# the apache xmlrpc client in the classpath (http://ws.apache.org/xmlrpc/client.html)

# map java classes from apach jar files to qore classes
%module-cmd(gnu-java) import org.apache.xmlrpc.client.*
%module-cmd(gnu-java) import java.util.HashMap

# require all variables to be declared
%require-our

# instantiate the getStatus2 class as the application class
%exec-class getStatus2

class getStatus2 {
    const server_url = "http://localhost:8001";
 
    constructor() {
	# set server URL
	my string $server_url = strlen($ARGV[0]) ? $ARGV[0] : server_url;

	# create XML-RPC configuration
	my org::apache::xmlrpc::client::XmlRpcClientConfigImpl $config();

	# create URL object
	my java::net::URL $url($server_url);
	printf("url=%s\n", $url.toString());

	# set URL in config
	$config.setServerURL($url);
	
	# create XML-RPC client oject
	my org::apache::xmlrpc::client::XmlRpcClient $client();

	# apply config (URL) to client object
	$client.setConfig($config);

	# call the method on the Qorus server and get the result
	my HashMap $result = $client.execute("omq.system.get-status", ());

	# get fields from the result
	my string $instance_key = $result.get("instance-key");
	my int $sessionid = $result.get("session-id");
	my string $version = $result.get("omq-version");

	# output the result
	printf("Qorus " + $version + " " + $instance_key + " sessionid " + $sessionid + "\n");
    }
}
