// IIslEngine.cs — host interface. JS IslEngine is authoritative for browser.
// Status: interface contract.

using SovereignX.CIEMS.Engine.Runtime;

namespace SovereignX.CIEMS.Engine.Scripting
{
    public interface IIslEngine
    {
        IntentRecord CompileAndEvaluate(string islSource, string contextJson);
    }
}
