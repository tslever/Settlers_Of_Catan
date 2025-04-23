const URL_OF_BACK_END = "http://localhost:5000";

export const API = {
    baseUrl: URL_OF_BACK_END,
    endpoints: {
        cities: '/cities',
        automateMove: '/automateMove',
        reset: '/reset',
        roads: '/roads',
        settlements: '/settlements',
        state: '/state',
        walls: '/walls'
    }
};

export async function apiFetch<T>(
    endpoint: string,
    options?: RequestInit
): Promise<T> {
    const response = await fetch(`${API.baseUrl}${endpoint}`, options);
    if (!response.ok) {
        let errorMessage = response.statusText;
        try {
            const errorData = await response.json();
            errorMessage = errorData.error || errorMessage;
        } catch (err) {
            // TODO: Consider whether falling back to status text if JSON parsing fails requires code here.
        }
        throw new Error(`API Error: ${errorMessage}`);
    }
    return response.json();
}