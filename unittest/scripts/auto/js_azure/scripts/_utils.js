function mysqlsh(options, envvars){
    testutil.callMysqlsh(options, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"].concat(envvars))
}

exports.mysqlsh = mysqlsh;

exports.getConnectionString = function(endpoint, full, account='', key=''){
    let emulator = endpoint.indexOf("127.0.0.1") != -1
    if (full) {
        if (emulator) {
            return `DefaultEndpointsProtocol=http;AccountName=${account};AccountKey=${key};BlobEndpoint=${endpoint};`;
        } else {
            return `DefaultEndpointsProtocol=https;AccountName=${account};AccountKey=${key};EndpointSuffix=core.windows.net`;
        }
    } else {
        if (emulator) {
            return `DefaultEndpointsProtocol=http;BlobEndpoint=${endpoint};`;
        } else {
            return `DefaultEndpointsProtocol=https;EndpointSuffix=core.windows.net`;
        }
    }
}

// NOTE: The order in which these functions are returned must not be changed as the index will be used for custom error messages
function getDumpAndLoadFunctionCallbacks(options, envvars){
    return {
        dumpInstance:function(){mysqlsh([__mysqluripwd + "/mysql", "--", "util", "dump-instance", "folder"].concat(options), envvars)},
        dumpSchemas:function(){mysqlsh([__mysqluripwd + "/mysql", "--", "util", "dump-schemas", "sakila", "--output-url=folder"].concat(options), envvars)},
        dumpTables:function(){mysqlsh([__mysqluripwd + "/mysql", "--", "util", "dump-tables", "sakila", "actor", "--output-url=folder"].concat(options), envvars)},
        exportTable:function(){mysqlsh([__mysqluripwd + "/mysql", "--", "util", "export-table", "sakila.actor", "folder"].concat(options), envvars)},
        importTable:function(){mysqlsh([__mysqluripwd + "/mysql", "--", "util", "import-table", "path"].concat(options), envvars)},
        loadDump:function(){mysqlsh([__mysqluripwd + "/mysql", "--", "util", "load-dump", "sample"].concat(options), envvars)}
    };
}

exports.testFailure = function (options, envvars, error, custom_errors={}){
    let functions = getDumpAndLoadFunctionCallbacks(options, envvars);
    for(index in functions){
        expected_error = error;
        if (index in custom_errors) {
            expected_error = custom_errors[index];
        }

        functions[index]();
        EXPECT_OUTPUT_CONTAINS(expected_error);
        WIPE_OUTPUT();
    }
}
