#if defined _OutputInfo_Included
 #endinput
#endif
#define _OutputInfo_Included

native int GetOutputCount(int Entity, const char[] sOutput);
native int GetOutputTarget(int Entity, const char[] sOutput, int Index, char[] sTarget);
native int GetOutputTargetInput(int Entity, const char[] sOutput, int Index, char[] sTargetInput);
native int GetOutputParameter(int Entity, const char[] sOutput, int Index, char[] sParameter);
native float GetOutputDelay(int Entity, const char[] sOutput, int Index);

/**
 * Do not edit below this line!
 */
public Extension __ext_outputinfo =
{
	name = "OutputInfo",
	file = "outputinfo.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public __ext_outputinfo_SetNTVOptional()
{
	MarkNativeAsOptional("GetOutputCount");
	MarkNativeAsOptional("GetOutputTarget");
	MarkNativeAsOptional("GetOutputTargetInput");
	MarkNativeAsOptional("GetOutputParameter");
	MarkNativeAsOptional("GetOutputDelay");
}
#endif
