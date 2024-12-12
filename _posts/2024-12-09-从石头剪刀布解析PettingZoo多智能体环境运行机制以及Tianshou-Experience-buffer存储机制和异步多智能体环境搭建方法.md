---
title: 从石头剪刀布解析 PettingZoo 多智能体环境运行机制以及 Tianshou Experience buffer 存储机制和异步多智能体环境搭建方法
date: 2024-12-09 08:56:45 +0800
categories: [软件]
tags: [drl, pettingzoo, tianshou]    # TAG names should always be lowercase
---

## 前言

[PettingZoo](https://pettingzoo.farama.org/) 存在 [AEC API](https://pettingzoo.farama.org/api/aec/) 和 [Parallel API](https://pettingzoo.farama.org/api/parallel/) 两种运行模式。

在 [Parallel API](https://pettingzoo.farama.org/api/parallel/) 模式的环境中，所有智能体同时进行决策，环境根据**所有**智能体的动作进行状态转移。如果你将 Parallel API 的[示例代码](https://pettingzoo.farama.org/api/parallel/#usage)与 [Gymnasium](https://gymnasium.farama.org/) 单智能体环境的[示例代码](https://gymnasium.farama.org/introduction/basic_usage/#interacting-with-the-environment)进行对比，你会发现两者的代码结构完全一样：

```python
# PettingZoo Parallel API
from pettingzoo.butterfly import pistonball_v6
parallel_env = pistonball_v6.parallel_env(render_mode="human")
observations, infos = parallel_env.reset(seed=42)

while parallel_env.agents:
    # this is where you would insert your policy
    actions = {agent: parallel_env.action_space(agent).sample() for agent in parallel_env.agents}

    observations, rewards, terminations, truncations, infos = parallel_env.step(actions)
parallel_env.close()
```

```python
# Gymnasium
import gymnasium as gym
env = gym.make("LunarLander-v3", render_mode="human")
observation, info = env.reset()

episode_over = False
while not episode_over:
    # agent policy that uses the observation and info
    action = env.action_space.sample()

    observation, reward, terminated, truncated, info = env.step(action)
    episode_over = terminated or truncated
env.close()
```

两者都是通过 `reset` 方法初始化环境并且获得初始 `observation` 。然后在循环内部，智能体先进行决策产生 `action` ，然后调用 `step` 方法完成环境状态转移，得到本次 `action` 的 `reward` 以及下一次观测 `observation` 、`terminate` 和 `truncate` 。两者唯一的区别在于：Gymnasium 单智能体产生 `action` 的位置变成了 Parallel API 所有智能体同时产生 `action` 。

![PettingZooParallelAPI-vs-Gymnasium](/assets/img/PettingZoo/ParallelAPI-vs-Gymnasium.drawio.png)

通过与 Gymnasium 对比，理解 PettingZoo Parallel API 的运行机制应该会更加容易。具体如何编写 PettingZoo Parallel API 环境请参照[这个教程](https://pettingzoo.farama.org/tutorials/custom_environment/)和[这个代码](https://pettingzoo.farama.org/content/environment_creation/#example-custom-parallel-environment)。

但是如果你想用多智能体解决更加复杂的问题，就很有可能需要设计一个更加复杂的多智能体环境，比如：

- 智能体在环境中需要依次决策，而不是同时决策
- 下一个智能体需要根据上一个智能体的决策导致的环境变化进行决策
- 上一个智能体的决策结果需要传递给下一个智能体
- 上一个智能体的决策会决定下一个进行决策的智能体是谁
- 等等一系列奇思妙想……

此时 Parallel API 的局限性不言而喻。

在接下来的部分，我们首先分析基于 [AEC API](https://pettingzoo.farama.org/api/aec/) 的[石头剪刀布环境代码](https://pettingzoo.farama.org/content/environment_creation/#example-custom-environment)，分析其运行机制。然后使用 [Tianshou](https://tianshou.org/en/stable/) 的[多智能体强化学习框架](https://tianshou.org/en/stable/01_tutorials/04_tictactoe.html)与石头剪刀布 AEC API 环境进行交互，解析其 Experience buffer 存储机制。最后，通过魔改 AEC API 石头剪刀布环境，探究异步多智能体环境搭建方法。

## PettingZoo AEC API 石头剪刀布环境运行机制

与使用 AEC API 的环境交互的[示例代码](https://pettingzoo.farama.org/api/aec/#usage)如下：

```python
from pettingzoo.classic import rps_v2

env = rps_v2.env(render_mode="human")
env.reset(seed=42)

for agent in env.agent_iter():
    observation, reward, termination, truncation, info = env.last()

    if termination or truncation:
        action = None
    else:
        # this is where you would insert your policy
        action = env.action_space(agent).sample()

    env.step(action)
env.close()
```

从中可以看出，AEC 环境使用 `reset` 初始化后不会返回初始 `observation` ，而是在循环内部使用 `last` 方法获取当前观测 `observation` 、`termination` 和 `truncation` 以及上一次 `action` 的 `reward` 。智能体决策产生 `action` 后调用 `step` 方法完成环境状态转移，与 `reset` 一样没有任何返回值。

在后面的小节中我们将分析 PettingZoo AEC API 剪刀石头布环境的[代码](https://pettingzoo.farama.org/content/environment_creation/#example-custom-environment)，研究其选择 `agent` 以及生成对应 `agent` 的 `observation` 和 `reward` 的机制。

### 函数分析

#### `__init__`

`__init__` 函数定义了环境中不变的固有参数：

- `self.possible_agents` 定义了在环境中可能出现的**全部** agent
- `self.agent_name_mapping` 给所有 agent 定义了索引
- `self.action_spaces` 和 `self.observation_spaces` 定义了**全部**智能体的动作空间和观测空间
- `self.render_mode` 定义了环境的渲染模式

#### `observation_space` 和 `action_space`

根据 `AEC` 源码 [L113](https://github.com/Farama-Foundation/PettingZoo/blob/master/pettingzoo/utils/env.py#L113) 和 [L125](https://github.com/Farama-Foundation/PettingZoo/blob/master/pettingzoo/utils/env.py#L125) ，`observation_space` 和 `action_space` 方法应当根据传入的 `agent` 参数返回对应的观测空间和动作空间。通常从 `self.observation_spaces` 和 `self.action_spaces` 字典中获取。

#### `render`

`render` 方法用于渲染环境。在 `human` 模式下，会打印当前环境状态。

#### `observe`

`observe` 返回传入 `agent` 的观测，从储存了所有智能体的观测的字典 `self.observations` 中获取。

#### `reset`

`reset` 方法用于初始化所有动态变化的环境参数（与 `__init__` 中定义的参数不同）。其中包括：

- `self.agents` 定义了当前参与决策的智能体（是 `self.possible_agents` 的子集）
- `self.rewards` 为最后一个智能体执行完动作后，环境所产生的所有智能体的奖励
- `self._cumulative_rewards` 为每个智能体的累积奖励
- `self.terminations` 、`self.truncations` 和 `self.infos` 用于记录所有智能体的状态
- `self.state` 用于记录当前环境的**全局状态**
- `self.observations` 用于记录所有智能体的观测
- `self.num_moves` 用于记录当前环境的步数
- `self._agent_selector` 是一个用 `self.agents` 初始化的迭代器，用于选择下一个智能体
- `self.agent_selection` 记录了下一次调用 `step` 方法的智能体

#### `step`

`step` 方法是环境根据智能体动作进行状态转移的核心函数。其执行逻辑如下：

1. 检查 `self.agent_selection` 的状态是否为 `terminated` 或 `truncated` ，如果是，则执行 `self._was_dead_step` 方法处理这些已经结束的智能体。通过查看[源码](https://github.com/Farama-Foundation/PettingZoo/blob/master/pettingzoo/utils/env.py#L195)了解该方法具体完成了什么操作。
2. 清空该智能体的累计奖励 `self._cumulative_rewards[self.agent_selection]` 以便在后面的代码中重新计算（提示：`last` 方法返回的奖励来自于 `self._cumulative_rewards` ）。
3. 使用该智能体的动作更新全局状态 `self.state`
4. 接下来的代码执行与当前智能体有关
   - 如果当前智能体是第一个智能体，说明还需要等待第二个智能体做出动作，才能知道本轮猜拳的奖励
     1. 所以更新全局状态 `self.state` 标记第二个智能体尚未做出动作
     2. 调用 `self._clear_rewards` 清空上一轮猜拳的奖励
   - 如果当前智能体是第二个智能体，那么就可以计算本轮猜拳的奖励了
     1. 设置奖励 `self.rewards`
     2. 更新环境状态 `self.num_moves`
     3. 更新智能体状态 `self.truncations`
     4. 更新智能体观测 `self.observations`
5. 处理完当前智能体的动作后，设置 `self.agent_selection` 为下一个智能体
6. 调用 `self._accumulate_rewards` 方法更新智能体的累计奖励，以供 `last` 方法返回
7. 调用 `self.render` 方法渲染环境

### 小结

从 `step` 函数的逻辑中可以看出，`AEC` 环境的运行机制是：所有参与决策的智能体按照 agent selector 迭代器的顺序依次决策。每个智能体执行动作后，环境都会发生变化，并且产生一个针对所有智能体的奖励 `self.rewards` 。这些奖励都会积累在 `self._cumulative_rewards` 中。当所有参与决策的智能体都执行完动作后，环境会将累计奖励分发给对应的智能体。在下一轮决策开始时，累计奖励 `self._cumulative_rewards` 需要清零重新计算。

![PettingZooAECAPI](/assets/img/PettingZoo/AECAPI.drawio.png)

## Tianshou Experience buffer 在 PettingZoo AEC 环境的存储机制

从上面对 AEC 环境代码的分析中可以看出：智能体做出决策和获取奖励并不是同步的（即：智能体做出决策后，环境无法立刻返回奖励），必须等到所有智能体动作全部执行完毕后，环境才会返回奖励。

那么 Tianshou 的多智能体强化学习框架是如何处理这种情况的呢？下面将使用 Tianshou 的多智能体强化学习框架与 PettingZoo AEC API 石头剪刀布环境进行交互，通过单步调试观察多智能体与环境交互时 Experience buffer 的变化，来解析 Tianshou Experience buffer 在 PettingZoo AEC 环境的存储机制。

### 测试代码

将石头剪刀布 AEC 环境[代码](https://pettingzoo.farama.org/content/environment_creation/#example-custom-environment)保存为 `aec_rps.py` ，并在同级目录下创建 `main.py` 文件，在其中编写代码实现两个 DQN policy 进行石头剪刀布游戏。代码参考[这个](https://github.com/liuzhaoze/Tianshou-MARL/blob/main/tianshou-tictactoe.py)和[这个](https://github.com/thu-ml/tianshou/blob/master/test/pettingzoo/tic_tac_toe.py)，省略了测试 `test` 相关的代码。

> 笔者认为 `aec_rps.py` 中的 [`observe` 方法](https://github.com/Farama-Foundation/PettingZoo/blob/master/docs/code_examples/aec_rps.py#L118)应当返回一个 OneHot 向量，以适配神经网络的输入，而非一个标量。因此暂时做出如下修改：
>
> ```python
> def observe(self, agent):
>     return np.eye(4)[self.observations[agent]]
> ```

```python
import argparse
from copy import deepcopy
from functools import partial

import gymnasium
import numpy as np
import torch
from tianshou.data import Collector, VectorReplayBuffer
from tianshou.env import DummyVectorEnv
from tianshou.env.pettingzoo_env import PettingZooEnv
from tianshou.policy import BasePolicy, DQNPolicy, MultiAgentPolicyManager
from tianshou.trainer import OffpolicyTrainer
from tianshou.utils.net.common import Net

import aec_rps


def get_env(render_mode: str | None = None) -> PettingZooEnv:
    return PettingZooEnv(aec_rps.env(render_mode=render_mode))


def get_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()

    parser.add_argument("--device", type=str, default="cuda" if torch.cuda.is_available() else "cpu")
    parser.add_argument("--seed", type=int, default=0)

    # Parameters for DRL
    parser.add_argument("--hidden-sizes", type=int, nargs="*", default=[128, 128], help="Hidden layer sizes of DQN.")
    parser.add_argument("--learning-rate", type=float, default=0.001)
    parser.add_argument("--gamma", type=float, default=0.99, help="Discount factor.")
    parser.add_argument("--td-step", type=int, default=3, help="Number of steps for multi-step TD learning.")
    parser.add_argument("--target-update-freq", type=int, default=320, help="Frequency of target network update.")
    parser.add_argument("--buffer-size", type=int, default=6, help="Size of replay buffer.")
    parser.add_argument("--epsilon-train", type=float, default=1.0, help="Epsilon for training.")
    parser.add_argument("--epsilon-test", type=float, default=0.05, help="Epsilon for testing.")
    parser.add_argument("--batch-size", type=int, default=64)

    # Parameters for training
    parser.add_argument("--render-mode", type=str, default=None)
    parser.add_argument("--train-env-num", type=int, default=1, help="Number of environments for parallel training.")
    parser.add_argument("--test-env-num", type=int, default=1, help="Number of environments for testing the policy.")
    parser.add_argument("--epoch-num", type=int, default=10, help="Number of epochs for training.")
    parser.add_argument("--step-per-epoch", type=int, default=100, help="Number of transitions collected per epoch.")
    parser.add_argument(
        "--step-per-collect",
        type=int,
        default=1,
        help="Number of transitions the collector would collect before the network update",
    )
    parser.add_argument(
        "--update-per-step",
        type=float,
        default=1.0,
        help="How many gradient steps to perform per step in the environment.\n1.0 means perform one gradient step per environment step.\n0.1 means perform one gradient step per 10 environment steps.",
    )
    parser.add_argument("--episode-per-test", type=int, default=1, help="Number of episodes for one policy evaluation.")

    if len(parser.parse_known_args()[1]) > 0:
        print("Unknown arguments: ", parser.parse_known_args()[1])

    return parser.parse_known_args()[0]


def get_env_info(args: argparse.Namespace) -> argparse.Namespace:
    env = get_env()
    args.state_space = (
        env.observation_space["observation"]
        if isinstance(env.observation_space, gymnasium.spaces.Dict)
        else env.observation_space
    )  # `env.observation_space` may has `action_mask` key
    args.state_shape = args.state_space.shape or int(args.state_space.n)
    args.action_space = env.action_space
    args.action_shape = args.action_space.shape or int(args.action_space.n)
    return args


def get_policy(
    args: argparse.Namespace,
    policy_self: BasePolicy | None = None,
    policy_opponent: BasePolicy | None = None,
    optimizer: torch.optim.Optimizer | None = None,
):
    if policy_self is None:
        net = Net(
            state_shape=args.state_shape,
            action_shape=args.action_shape,
            hidden_sizes=args.hidden_sizes,
            device=args.device,
        ).to(args.device)

        if optimizer is None:
            optimizer = torch.optim.Adam(net.parameters(), lr=args.learning_rate)

        policy_self = DQNPolicy(
            model=net,
            optim=optimizer,
            action_space=args.action_space,
            discount_factor=args.gamma,
            estimation_step=args.td_step,
            target_update_freq=args.target_update_freq,
        )

    if policy_opponent is None:
        policy_opponent = deepcopy(policy_self)

    env = get_env()
    ma_policy = MultiAgentPolicyManager(policies=[policy_self, policy_opponent], env=env)

    return ma_policy, env.agents


def train(
    args: argparse.Namespace,
    policy_self: BasePolicy | None = None,
    policy_opponent: BasePolicy | None = None,
    optimizer: torch.optim.Optimizer | None = None,
):
    train_envs = DummyVectorEnv([partial(get_env, render_mode=args.render_mode) for _ in range(args.train_env_num)])
    test_envs = DummyVectorEnv([partial(get_env, render_mode=args.render_mode) for _ in range(args.test_env_num)])

    # seed
    np.random.seed(args.seed)
    torch.manual_seed(args.seed)
    train_envs.seed(args.seed)
    test_envs.seed(args.seed)

    # policy
    ma_policy, agents = get_policy(args, policy_self=policy_self, policy_opponent=policy_opponent, optimizer=optimizer)

    # replay buffer
    buffer = VectorReplayBuffer(args.buffer_size, args.train_env_num)

    # collector
    train_collector = Collector(ma_policy, train_envs, buffer, exploration_noise=True)
    test_collector = Collector(ma_policy, test_envs, exploration_noise=True)

    # train
    def train_fn(num_epoch: int, step_idx: int) -> None:
        [agent.set_eps(args.epsilon_train) for agent in ma_policy.policies.values()]

    def test_fn(num_epoch: int, step_idx: int) -> None:
        [agent.set_eps(args.epsilon_test) for agent in ma_policy.policies.values()]

    def stop_fn(mean_rewards: float) -> bool:
        return False

    def save_best_fn(policy: BasePolicy) -> None:
        pass

    def reward_metric(rewards: np.ndarray) -> np.ndarray:
        return rewards[:, 0]

    result = OffpolicyTrainer(
        policy=ma_policy,
        max_epoch=args.epoch_num,
        batch_size=args.batch_size,
        train_collector=train_collector,
        test_collector=test_collector,
        step_per_epoch=args.step_per_epoch,
        episode_per_test=args.episode_per_test,
        update_per_step=args.update_per_step,
        step_per_collect=args.step_per_collect,
        train_fn=train_fn,
        test_fn=test_fn,
        stop_fn=stop_fn,
        save_best_fn=save_best_fn,
        reward_metric=reward_metric,
        test_in_train=False,
    ).run()

    return result, ma_policy


if __name__ == "__main__":
    args = get_args()
    args = get_env_info(args)
    result, policy = train(args)
    print(result)
```

### 调试方法

使用 VSCode 的 Python 调试配置 `launch.json` 如下：

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Python 调试程序: 包含参数的当前文件",
            "type": "debugpy",
            "request": "launch",
            "program": "${workspaceFolder}/main.py",
            "console": "integratedTerminal",
            "args": []
        }
    ]
}
```

#### 找到 `OffpolicyTrainer` 中训练时执行 `step` 的位置

1. 在 `OffpolicyTrainer` 的 `run` 处转到定义 <kbd>F12</kbd> ，进入 `run` 方法
2. 在 `run` 方法中 `deque(self, maxlen=0)` 处打断点 <kbd>F9</kbd> ，在训练时智能体在此处与环境交互
3. 运行调试 <kbd>F5</kbd> 程序在 `deque(self, maxlen=0)` 处停止
4. 单步调试 <kbd>F11</kbd> ，进入 `__iter__` 方法，此处用于重置 `OffpolicyTrainer` 的状态
5. 单步跳出 <kbd>Shift</kbd> + <kbd>F11</kbd> ，返回 `deque(self, maxlen=0)` 处
6. 单步调试 <kbd>F11</kbd> ，进入 `__next__` 方法，找到 `train_stat, update_stat, self.stop_fn_flag = self.training_step()` 行打断点 <kbd>F9</kbd> 此处为训练时执行 `step` 的位置
7. 找到该位置后，即可删除 `deque(self, maxlen=0)` 处的断点

#### 找到 Tianshou `PettingZooEnv` 中 `step` 方法的位置

1. 在 `PettingZooEnv` 处转到定义 <kbd>F12</kbd> ，进入 `PettingZooEnv` 类
2. 找到该类中的 `step` 方法，在 `self.env.step(action)` 处打断点 <kbd>F9</kbd> 此处为环境执行 `step` 的位置
3. 禁用该断点，因为 Tianshou 在开始训练前会测试环境，如果不禁用该断点，程序会在不必要的时候暂停

#### 正式进行单步调试

1. 运行调试 <kbd>F5</kbd> 程序在 `train_stat, update_stat, self.stop_fn_flag = self.training_step()` 处暂停
2. 启用之前禁用的 `PettingZooEnv` 中 `step` 方法的断点
3. 继续调试 <kbd>F5</kbd> ，程序在 `PettingZooEnv` 中 `step` 方法处暂停

如果你想查看环境中具体的变化，可以单步调试 <kbd>F11</kbd> ，此时就进入了 `aec_rps.py` 中 `raw_env` 的 `step` 方法。可以单步跳出 <kbd>Shift</kbd> + <kbd>F11</kbd> ，返回 `PettingZooEnv` 中 `step` 方法。接着继续调试 <kbd>F5</kbd> ，程序会回到 `OffpolicyTrainer` 的 `__next__` 方法的 `train_stat, update_stat, self.stop_fn_flag = self.training_step()` 行，准备下一次决策。

如果你想直接跳转到下一次决策之前，可以继续调试 <kbs>F5</kbd> ，程序会从 `PettingZooEnv` 的 `step` 方法直接跳回 `OffpolicyTrainer` 的 `__next__` 方法的 `train_stat, update_stat, self.stop_fn_flag = self.training_step()` 行。

无论用上述两段中的哪一种方法执行一轮 `train_stat, update_stat, self.stop_fn_flag = self.training_step()` 后，进入调用堆栈中 `main.py` 的 `train` 方法，都可以通过监视 `len(buffer)` 和 `print(buffer)` 观察到 Experience buffer 增加了记录。

### 结果分析

为了方便观察，`--buffer-size` 的默认值设置为 6 。为了能观察到不同的情况，`--epsilon-train` 的默认值设置为 1.0 全随机。

环境每一步的状态变化如下表所示：

<table>
    <thead align="center" valign="center">
        <tr>
            <td rowspan="2">Current Agent</td>
            <td rowspan="2">Last Observation</td>
            <td rowspan="2">Action</td>
            <td colspan="2">Rewards</td>
            <td rowspan="2">备注</td>
            <td colspan="2">Cumulative Rewards</td>
            <td rowspan="2">备注</td>
        </tr>
        <tr>
            <td>agent_0</td>
            <td>agent_1</td>
            <td>agent_0</td>
            <td>agent_1</td>
        </tr>
    </thead>
    <tbody align="center" valign="center">
        <tr>
            <td>player_0</td>
            <td>3</td>
            <td>2</td>
            <td>0</td>
            <td>0</td>
            <td>调用clear_rewards清空</td>
            <td>0</td>
            <td>0</td>
            <td>清零当前智能体累计奖励后累加Rewards</td>
        </tr>
        <tr>
            <td>player_1</td>
            <td>3</td>
            <td>1</td>
            <td>1</td>
            <td>-1</td>
            <td>所有智能体执行动作后计算结果</td>
            <td>1</td>
            <td>-1</td>
            <td>清零当前智能体累计奖励后累加Rewards</td>
        </tr>
        <tr>
            <td>player_0</td>
            <td>1</td>
            <td>1</td>
            <td>0</td>
            <td>0</td>
            <td>调用clear_rewards清空</td>
            <td>0</td>
            <td>-1</td>
            <td>清零当前智能体累计奖励后累加Rewards</td>
        </tr>
        <tr>
            <td>player_1</td>
            <td>2</td>
            <td>2</td>
            <td>-1</td>
            <td>1</td>
            <td>所有智能体执行动作后计算结果</td>
            <td>-1</td>
            <td>1</td>
            <td>清零当前智能体累计奖励后累加Rewards</td>
        </tr>
        <tr>
            <td>player_0</td>
            <td>2</td>
            <td>1</td>
            <td>0</td>
            <td>0</td>
            <td>调用clear_rewards清空</td>
            <td>0</td>
            <td>1</td>
            <td>清零当前智能体累计奖励后累加Rewards</td>
        </tr>
        <tr>
            <td>player_1</td>
            <td>1</td>
            <td>1</td>
            <td>0</td>
            <td>0</td>
            <td>所有智能体执行动作后计算结果</td>
            <td>0</td>
            <td>0</td>
            <td>清零当前智能体累计奖励后累加Rewards</td>
        </tr>
    </tbody>
</table>

Replay buffer 的内容如下：

```text
VectorReplayBuffer(
    done: array([False, False, False, False, False, False]),
    rew: array([[ 0.,  0.],
                [ 1., -1.],
                [ 0.,  0.],
                [-1.,  1.],
                [ 0.,  0.],
                [ 0.,  0.]]),
    policy: Batch(
                hidden_state: Batch(
                                  player_0: Batch(),
                                  player_1: Batch(),
                              ),
            ),
    truncated: array([False, False, False, False, False, False]),
    act: array([2, 1, 1, 2, 1, 1]),
    terminated: array([False, False, False, False, False, False]),
    obs_next: Batch(
                  mask: array([[ True,  True,  True],
                               [ True,  True,  True],
                               [ True,  True,  True],
                               [ True,  True,  True],
                               [ True,  True,  True],
                               [ True,  True,  True]]),
                  agent_id: array(['player_1', 'player_0', 'player_1', 'player_0', 'player_1',
                                   'player_0'], dtype=object),
                  obs: array([[0., 0., 0., 1.],
                              [0., 1., 0., 0.],
                              [0., 0., 1., 0.],
                              [0., 0., 1., 0.],
                              [0., 1., 0., 0.],
                              [0., 1., 0., 0.]]),
              ),
    info: Batch(
              env_id: array([0, 0, 0, 0, 0, 0]),
          ),
    obs: Batch(
             mask: array([[ True,  True,  True],
                          [ True,  True,  True],
                          [ True,  True,  True],
                          [ True,  True,  True],
                          [ True,  True,  True],
                          [ True,  True,  True]]),
             agent_id: array(['player_0', 'player_1', 'player_0', 'player_1', 'player_0',
                              'player_1'], dtype=object),
             obs: array([[0., 0., 0., 1.],
                         [0., 0., 0., 1.],
                         [0., 1., 0., 0.],
                         [0., 0., 1., 0.],
                         [0., 0., 1., 0.],
                         [0., 1., 0., 0.]]),
         ),
)
```

可以看出 Replay buffer 中按照 `agent_id` 区分了不同智能体的观测和动作。每条记录中保存了当前 step 所有智能体的奖励，无论是否所有智能体都执行了动作。

## 异步多智能体环境搭建方法

目前的环境是 `player_0` 和 `player_1` 依次执行动作。如果我们要设计的环境中每个 `step` 智能体会发生变化，而且执行决策的次数也不一致，那么应该如何实现？

下面通过魔改 PettingZoo AEC API 石头剪刀布环境为例，说明异步多智能体环境的搭建方法。

### 环境设计

假设环境中存在四个智能体 `player_0` 、`player_1` 、`player_2` 和 `player_3` ，猜拳的次序为：`player_0` 和 `player_1` 比一轮，`player_2` 和 `player_3` 比两轮，以此循环。

### 代码实现

对于智能体可变的环境，需要参考 [Knights Archers Zombies](https://pettingzoo.farama.org/environments/butterfly/knights_archers_zombies/) 环境的[代码](https://github.com/Farama-Foundation/PettingZoo/blob/master/pettingzoo/butterfly/knights_archers_zombies/knights_archers_zombies.py)。

改动代码的关键在于：在 `step` 函数判断 `self._agent_selector.is_last()` ([L750](https://github.com/Farama-Foundation/PettingZoo/blob/master/pettingzoo/butterfly/knights_archers_zombies/knights_archers_zombies.py#L750)) 中，使用 `self._agent_selector.reinit()` ([L765](https://github.com/Farama-Foundation/PettingZoo/blob/master/pettingzoo/butterfly/knights_archers_zombies/knights_archers_zombies.py#L765)) 设定下一次参与猜拳的两个智能体。在设置奖励时，不参与猜拳的智能体的奖励设为 0 。
