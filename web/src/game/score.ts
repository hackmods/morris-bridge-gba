export function starsForCost(cost: number, par: number): number {
  if (cost <= par) {
    return 3;
  }
  if (cost <= par * 1.35) {
    return 2;
  }
  return 1;
}

export function formatCost(cost: number): string {
  return `£${Math.round(cost).toLocaleString('en-GB')}`;
}
