import { defineConfig } from 'vitepress'

export default defineConfig({
  title: 'Smart Cap',
  base: '/',
  description:
    'Developer documentation for the Smart Cap assistive-device prototype.',
  cleanUrls: true,
  lastUpdated: true,
  themeConfig: {
    siteTitle: 'Smart Cap',
    nav: [
      { text: 'Guide', link: '/getting-started/introduction' },
      { text: 'Architecture', link: '/architecture/overview' },
      { text: 'Hardware', link: '/hardware/board' },
      { text: 'Test results', link: '/testing/results' },
    ],
    socialLinks: [
      {
        icon: 'github',
        link: 'https://github.com/abhishek9773/smart-cap',
        ariaLabel: 'Smart Cap on GitHub',
      },
    ],
    search: {
      provider: 'local',
    },
    sidebar: [
      {
        text: 'Getting Started',
        items: [
          { text: 'Introduction', link: '/getting-started/introduction' },
          { text: 'Project status', link: '/getting-started/status' },
          { text: 'Hardware requirements', link: '/getting-started/hardware' },
          { text: 'Development environment', link: '/getting-started/environment' },
          { text: 'Build, flash & monitor', link: '/getting-started/build-flash' },
        ],
      },
      {
        text: 'Architecture',
        items: [
          { text: 'System overview', link: '/architecture/overview' },
          { text: 'Project components', link: '/architecture/components' },
          { text: 'Camera driver', link: '/architecture/camera-driver' },
          { text: 'Camera pipeline', link: '/architecture/camera-pipeline' },
          { text: 'Image preprocessing', link: '/architecture/preprocessing' },
          { text: 'Memory management', link: '/architecture/memory' },
        ],
      },
      {
        text: 'Hardware',
        items: [
          { text: 'T-Camera S3 V1.6', link: '/hardware/board' },
          { text: 'Camera, OLED & PMU', link: '/hardware/peripherals' },
          { text: 'Pin configuration', link: '/hardware/pins' },
          { text: 'Hardware validation', link: '/hardware/validation' },
        ],
      },
      {
        text: 'Testing & Results',
        items: [
          { text: 'Validation procedure', link: '/hardware-test' },
          { text: 'Recorded results', link: '/testing/results' },
          { text: 'Current limitations', link: '/testing/limitations' },
        ],
      },
      {
        text: 'Development',
        items: [
          { text: 'Project structure', link: '/development/structure' },
          { text: 'Development workflow', link: '/development/workflow' },
          { text: 'Documenting new features', link: '/development/docs-policy' },
        ],
      },
    ],
    footer: {
      message: 'Open-source engineering prototype · Not production-ready',
      copyright: 'Documentation reflects implementation and recorded evidence.',
    },
  },
})
