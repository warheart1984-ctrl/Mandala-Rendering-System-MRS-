// mirrors engine/scripting/IIslEngine.cs
using GovernedEngine.Runtime;

namespace GovernedEngine.Scripting
{
    public interface IIslEngine
    {
        IntentRecord CompileAndEvaluate(string islSource, string contextJson);
    }
}
