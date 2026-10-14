import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "README.md"
PROJECT_BRIEF = (
    ROOT / "2110479 Computer Graphics — Term Project Template 2026 (1).pdf"
)


class RepositoryQualityTests(unittest.TestCase):
    def test_readme_has_project_title_and_scope(self):
        readme = README.read_text(encoding="utf-8")

        self.assertTrue(readme.startswith("# FluidBrushCG\n"))
        self.assertIn("resolution-independent fluid brush", readme)
        self.assertIn("C++ and Vulkan", readme)

    def test_readme_local_links_resolve(self):
        readme = README.read_text(encoding="utf-8")
        links = re.findall(r"\[[^\]]+\]\((?:<([^>]+)>|([^)]+))\)", readme)

        self.assertTrue(links, "README should link to project materials")
        for angle_target, plain_target in links:
            target = angle_target or plain_target
            if target.startswith(("https://", "http://", "mailto:")):
                continue
            self.assertTrue(
                (README.parent / target.split("#", 1)[0]).is_file(),
                f"README link does not resolve: {target}",
            )

    def test_project_brief_is_a_nonempty_pdf(self):
        self.assertTrue(PROJECT_BRIEF.is_file(), "Project brief is missing")
        with PROJECT_BRIEF.open("rb") as brief:
            self.assertEqual(brief.read(5), b"%PDF-", "Project brief is not a PDF")
            self.assertGreater(PROJECT_BRIEF.stat().st_size, 5)


if __name__ == "__main__":
    unittest.main()
