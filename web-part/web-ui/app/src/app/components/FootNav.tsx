import { ArrowUp } from 'lucide-react';

export function FootNav() {
  return (
    <div className="foot-nav">
      <button type="button" onClick={() => window.scrollTo({ top: 0, behavior: 'smooth' })}>
        <ArrowUp size={16} />
      </button>
    </div>
  );
}
