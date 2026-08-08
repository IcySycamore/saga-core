# Agent Context

本文件为项目上下文参考（人工可读），**状态一律以 `.agent/memory/state.json` 为准**，由 `statectl.py` 唯一维护，请勿在此重复记录状态字段，以免漂移。

## 最近修改的文件

last_modified_files: []

## 备注

- 查看当前状态：`python .agent/statectl.py current`
- 状态转换：`python .agent/statectl.py transition <state> [--retry] [--complete-task] [--interrupt]`
- 完整定义见 `.github/agents/orchestrator.agent.md` 与根目录 `AGENTS.md`
