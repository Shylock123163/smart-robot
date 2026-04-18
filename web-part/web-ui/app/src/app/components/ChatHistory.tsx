import { useEffect, useRef } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { User, Bot, AlertTriangle } from 'lucide-react';
import type { ChatItem } from '@/app/types';

type Props = {
  chatItems: ChatItem[];
};

const roleIcon = {
  user: User,
  assistant: Bot,
  system: AlertTriangle,
};

export function ChatHistory({ chatItems }: Props) {
  const bottomRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [chatItems.length]);

  return (
    <div className="chat-list">
      <AnimatePresence initial={false}>
        {chatItems.map((item, index) => {
          const Icon = roleIcon[item.role];
          return (
            <motion.div
              className={`chat-item chat-${item.role}`}
              key={`${item.role}-${index}`}
              initial={{ opacity: 0, y: 12 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ duration: 0.25 }}
            >
              <Icon size={14} className="chat-icon" />
              <span>{item.text}</span>
            </motion.div>
          );
        })}
      </AnimatePresence>
      <div ref={bottomRef} />
    </div>
  );
}
