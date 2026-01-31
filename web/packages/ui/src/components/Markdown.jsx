import { marked } from 'marked';

export function Markdown({ source }) {
    const html = marked.parse(source);

    return (
        <article
            class="markdown"
            dangerouslySetInnerHTML={{ __html: html }}
        />
    );
}
