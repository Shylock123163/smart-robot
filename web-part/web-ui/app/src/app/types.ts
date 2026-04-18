export type ChatItem = {
  role: 'assistant' | 'user' | 'system';
  text: string;
};

export type ResponseMeta = {
  aiSource: string;
  reason: string;
  commands: string[];
  memoryNotes: string[];
  fallback: boolean;
};

export const emptyResponseMeta: ResponseMeta = {
  aiSource: 'pending',
  reason: 'waiting',
  commands: [],
  memoryNotes: [],
  fallback: false,
};
