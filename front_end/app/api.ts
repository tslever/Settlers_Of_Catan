const URL_OF_BACK_END = "http://localhost:5000";

export const API = {
    baseUrl: URL_OF_BACK_END,
    endpoints: {
        cities: '/cities',
        automateMove: '/automateMove',
        makeMove: '/makeMove',
        recommendMove: '/recommendMove',
        reset: '/reset',
        roads: '/roads',
        settlements: '/settlements',
        state: '/state',
        walls: '/walls'
    }
};

export async function apiFetch<T>(
    endpoint: string,
    options: RequestInit = {}
): Promise<T> {
    const url = `${API.baseUrl}${endpoint}`;
    const method = options.method ?? "GET";
    const bodyPreview = options.body && (typeof options.body === "string" ? options.body : JSON.stringify(options.body));
    console.log(`[API REQUEST] ${method} ${url}`, bodyPreview ?? "(no-body)");
    try {
        const response = await fetch(url, options);
        if (!response.ok) {
            let payload: unknown = undefined;
            try {
                payload = await response.json();
            } catch {
                payload = await response.text();
            }
            console.error(`[API ERROR] ${response.status} ${url}`, payload);
            const message = typeof payload === "object" && payload && "error" in (payload as any) ? (payload as any).error : response.statusText;
            throw new Error(`API Error: ${message}`);
        }
        const data = await response.json() as T;
        console.log(`[API RESPONSE] ${response.status} ${url}`, data);
        return data;
    } catch (err) {
        console.error(`[API FETCH FAILED] ${url}`, err);
        throw err;
    }
}