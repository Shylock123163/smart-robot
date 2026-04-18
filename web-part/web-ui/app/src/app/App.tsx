import { useEffect, useMemo } from 'react';
import { Animator, Dots, GridLines, MovingLines } from '@arwes/react';
import { useRobotStore } from '@/stores/robotStore';
import { useOpenClawStatus } from '@/app/hooks/useOpenClawStatus';
import { useOpenClawChat } from '@/app/hooks/useOpenClawChat';
import { HeroPanel } from '@/app/components/HeroPanel';
import { MissionOpsStrip } from '@/app/components/MissionOpsStrip';
import { TaskInput } from '@/app/components/TaskInput';
import { DecisionPanel } from '@/app/components/DecisionPanel';
import { ExecutionPreview } from '@/app/components/ExecutionPreview';
import { ChatHistory } from '@/app/components/ChatHistory';
import { ScenePanel } from '@/app/components/ScenePanel';
import { DeviceStatus } from '@/app/components/DeviceStatus';
import { EventLog } from '@/app/components/EventLog';

export function App() {
  const {
    statusSummary,
    targetLabel,
    chassisMode,
    gripMode,
    sensorHealth,
    openClawRole,
    missionStatement,
  } = useRobotStore();

  const { serviceStatus, statusError, setStatusError, defaultChatMode } =
    useOpenClawStatus();

  const chat = useOpenClawChat();

  useEffect(() => {
    if (defaultChatMode) chat.setChatMode(defaultChatMode);
  }, [defaultChatMode]);

  useEffect(() => {
    if (serviceStatus) {
      chat.setTaskStage(
        serviceStatus.deviceConnected
          ? 'OpenClaw 在线 / 等待任务'
          : 'OpenClaw 在线 / 设备未连接',
      );
    } else if (statusError) {
      chat.setTaskStage('OpenClaw 状态异常');
    }
  }, [serviceStatus, statusError]);

  const eventLog = useMemo(() => [
    serviceStatus?.deviceConnected ? '设备已连接到 OpenClaw 队列' : '设备暂未连接到 OpenClaw 队列',
    `当前模式：${chat.chatMode === 'assistant' ? '助手模式' : '控制模式'}`,
    serviceStatus?.hasServerApiKey ? '服务器上游鉴权已配置' : '服务器上游鉴权尚未配置',
    serviceStatus?.executorMode ? `执行模式：${serviceStatus.executorMode}` : '执行模式待检测',
    `最近回复来源：${chat.responseMeta.aiSource === 'upstream' ? '上游模型' : chat.responseMeta.aiSource === 'pending' ? '等待中' : chat.responseMeta.aiSource}`,
  ], [chat.chatMode, chat.responseMeta.aiSource, serviceStatus]);

  const missionFlow = useMemo(() => [
    { title: '任务理解', state: chat.chatBusy ? '处理中' : '待命', detail: '从自然语言目标生成搜寻与夹取策略' },
    { title: '视觉搜寻', state: serviceStatus?.deviceConnected ? '可进入' : '设备未连接', detail: '进入床底/沙发底 ROI，等待目标稳定出现' },
    { title: '抓取执行', state: chat.responseMeta.commands.length ? `预览 ${chat.responseMeta.commands.length} 条命令` : '等待策略', detail: chat.responseMeta.commands.length ? chat.responseMeta.commands.join(' / ') : '尚未生成设备命令' },
    { title: '退出回收', state: chat.taskStage, detail: '夹到目标后退出低矮空间并回到用户可取位置' },
  ], [chat.chatBusy, chat.responseMeta.commands, serviceStatus?.deviceConnected, chat.taskStage]);

  const environmentCards = useMemo(() => [
    { label: '底盘姿态', text: `${chassisMode}，需要优先保证通过性与低矮空间机身余量` },
    { label: '夹持策略', text: `${gripMode}，更适合先对准中心线再闭合夹取` },
    { label: '传感器约束', text: `${sensorHealth}，进深搜索时要先保留安全退路` },
  ], [chassisMode, gripMode, sensorHealth]);

  const cameraOnline = serviceStatus?.deviceConnected ?? false;
  const sourceLabel = statusError ? '后端异常' : serviceStatus ? '服务已接通' : '检测中';
  const connectionSummary = serviceStatus
    ? `Provider ${serviceStatus.provider} / ${serviceStatus.endpoint}`
    : statusSummary;
  const nextAction = cameraOnline
    ? '设备在线，可继续下发高层任务并等待视觉 / 夹持执行反馈'
    : '先让 OpenClaw 在线并拉起设备队列，再进入真实夹取闭环';
  const robotImageUrl = `${import.meta.env.BASE_URL}robot.jpg`;

  function handleSend() {
    chat.handleSendChat(chat.currentTask, {
      robot: '智能巡拢家居机器人',
      target: targetLabel,
      chassisMode,
      gripMode,
      sensorHealth,
    });
    setStatusError('');
  }

  return (
    <div className="app-shell">
      <div className="app-bg">
        <GridLines lineColor="rgba(80, 214, 255, 0.12)" />
        <Dots color="rgba(88, 233, 196, 0.18)" />
        <MovingLines lineColor="rgba(255, 143, 77, 0.1)" />
      </div>

      <Animator active combine manager="stagger">
        <main className="dashboard">
          <HeroPanel
            currentTask={chat.currentTask}
            taskStage={chat.taskStage}
            targetLabel={targetLabel}
            sourceLabel={sourceLabel}
            robotImageUrl={robotImageUrl}
          />

          <MissionOpsStrip
            missionFlow={missionFlow}
            environmentCards={environmentCards}
            onSelectPreset={chat.setCurrentTask}
          />

          <section className="main-grid">
            <div className="left-column">
              <TaskInput
                currentTask={chat.currentTask}
                sessionId={chat.sessionId}
                chatMode={chat.chatMode}
                chatBusy={chat.chatBusy}
                onChangeTask={chat.setCurrentTask}
                onChangeChatMode={chat.setChatMode}
                onSend={handleSend}
                onResetSession={chat.resetSession}
              />
              <DecisionPanel
                openClawRole={openClawRole}
                nextAction={nextAction}
                missionStatement={missionStatement}
                connectionSummary={connectionSummary}
              />
              <ExecutionPreview responseMeta={chat.responseMeta} />
              <ChatHistory chatItems={chat.chatItems} />
            </div>

            <ScenePanel />

            <div className="right-column">
              <DeviceStatus
                cameraOnline={cameraOnline}
                chassisMode={chassisMode}
                gripMode={gripMode}
                sensorHealth={sensorHealth}
                pendingDeviceCommands={serviceStatus?.pendingDeviceCommands}
                activeSessions={serviceStatus?.activeSessions}
                statusLine={statusError || connectionSummary}
              />
              <EventLog eventLog={eventLog} />
            </div>
          </section>
        </main>
      </Animator>
    </div>
  );
}

